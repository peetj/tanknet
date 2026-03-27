#include <SDL.h>
#include <iostream>
#include <string>
#include <array>
#include <deque>
#include <chrono>
#include <thread>

#include "net/Protocol.h"
#include "net/NetCommon.h"
#include "client/Client.h"
#include "server/Server.h"
#include "render/Render.h"
#include "game/Math.h"
#include "game/Sim.h"
#include "game/World.h"

namespace tn {

static void usage() {
  std::cout << "tanknet --server --port 27015\n"
               "tanknet --client --host 127.0.0.1 --port 27015 --name P1\n";
}

struct Args {
  bool server{false};
  bool client{false};
  uint16_t port{27015};
  std::string host{"127.0.0.1"};
  std::string name{"P"};
};

static Args parse(int argc, char** argv) {
  Args a;
  for (int i=1;i<argc;i++) {
    std::string s = argv[i];
    auto next = [&](std::string def=""){
      if (i+1<argc) return std::string(argv[++i]);
      return def;
    };
    if (s == "--server") a.server = true;
    else if (s == "--client") a.client = true;
    else if (s == "--port") a.port = (uint16_t)std::stoi(next());
    else if (s == "--host") a.host = next();
    else if (s == "--name") a.name = next();
    else if (s == "--help" || s == "-h") { usage(); std::exit(0); }
  }
  return a;
}

static uint8_t encAxis(float v) {
  v = clampf(v, -1.0f, 1.0f);
  int iv = (int)std::lround(v * 127.0f) + 127;
  if (iv < 0) iv = 0;
  if (iv > 254) iv = 254;
  return (uint8_t)iv;
}

static Snapshot interp(const ClientView& a, const ClientView& b, float t) {
  Snapshot s = a.snap;
  for (int i=0;i<kMaxPlayers;i++) {
    s.players[i].pos.x = a.snap.players[i].pos.x + (b.snap.players[i].pos.x - a.snap.players[i].pos.x) * t;
    s.players[i].pos.y = a.snap.players[i].pos.y + (b.snap.players[i].pos.y - a.snap.players[i].pos.y) * t;
    // aim/hp/alive from newer
    s.players[i].aimRad = b.snap.players[i].aimRad;
    s.players[i].hp = b.snap.players[i].hp;
    s.players[i].alive = b.snap.players[i].alive;
  }
  // projectiles: just show newer for simplicity
  s.projectiles = b.snap.projectiles;
  s.tick = b.snap.tick;
  return s;
}

} // namespace tn

int main(int argc, char** argv) {
  using namespace tn;
  const Args args = parse(argc, argv);

  if (args.server) {
    NetConfig nc; nc.port = args.port;
    Server s(nc);
    if (!s.start()) return 2;
    s.run();
    return 0;
  }

  if (!args.client) { usage(); return 1; }

  NetConfig nc; nc.port = args.port; nc.host = args.host;
  Client net(nc, args.name);
  if (!net.connect()) return 2;

  Renderer rr;
  RenderConfig rc;
  if (!rr.init(rc)) {
    std::cerr << "SDL init failed\n";
    return 2;
  }

  // Local predicted sim (for instant local movement), reconciled to snapshots.
  Sim local;
  local.init();

  uint32_t myIndex = 0;
  bool running = true;

  uint32_t inputSeq = 0;
  struct SentInput { uint32_t seq; InputCmd cmd; Vec2 predictedPos; };
  std::deque<SentInput> sent;

  const uint64_t interpDelayMs = 80; // small buffer for jitter hiding

  uint64_t lastMs = nowMs();
  uint64_t lastInputSendMs = nowMs();

  while (running) {
    // Events
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) running = false;
      if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = false;
    }

    net.pumpNetwork();

    if (net.hasWelcome()) myIndex = net.playerIndex();

    // Build input (very low-latency: sample each frame; transmit at inputHz)
    const uint8_t* ks = SDL_GetKeyboardState(nullptr);
    float mx = 0.0f, my = 0.0f;
    if (ks[SDL_SCANCODE_A]) mx -= 1.0f;
    if (ks[SDL_SCANCODE_D]) mx += 1.0f;
    if (ks[SDL_SCANCODE_W]) my -= 1.0f;
    if (ks[SDL_SCANCODE_S]) my += 1.0f;

    int mouseX=0, mouseY=0;
    const uint32_t mb = SDL_GetMouseState(&mouseX, &mouseY);
    const bool shoot = (mb & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

    const Vec2 ppos = local.players[myIndex].pos;
    const float aim = std::atan2((float)mouseY - ppos.y, (float)mouseX - ppos.x);

    InputCmd cmd{};
    cmd.seq = ++inputSeq;
    cmd.moveX = encAxis(mx);
    cmd.moveY = encAxis(my);
    cmd.aimQ = (int16_t)std::lround(aim * 1000.0f);
    cmd.shoot = shoot ? 1 : 0;

    // Local prediction at fixed ticks (client tick)
    const uint64_t ms = nowMs();
    const uint64_t tickMs = 1000 / kTickHz;
    if (ms - lastMs >= tickMs) {
      lastMs += tickMs;
      std::array<InputCmd, kMaxPlayers> ins{};
      ins[myIndex] = cmd;
      local.step(1.0f/(float)kTickHz, ins);
      sent.push_back({cmd.seq, cmd, local.players[myIndex].pos});
      while (sent.size() > 256) sent.pop_front();
    }

    // Send input at inputHz
    const uint64_t inputPeriodMs = 1000 / std::max(1u, nc.inputHz);
    if (ms - lastInputSendMs >= inputPeriodMs) {
      lastInputSendMs = ms;
      net.sendInput(cmd);
    }

    // Pick a render snapshot (interpolated) from received snapshots
    Snapshot renderSnap = local.makeSnapshot();
    // Replace remote player/projectiles from server snapshots; reconcile local player if drift.
    const auto& views = net.snapshots();
    if (views.size() >= 2) {
      const uint64_t target = ms - interpDelayMs;

      int bIndex = -1;
      for (int i=(int)views.size()-1; i>=0; i--) {
        if (views[i].recvMs <= target) { bIndex = i; break; }
      }
      if (bIndex >= 0 && bIndex+1 < (int)views.size()) {
        const auto& a = views[bIndex];
        const auto& b = views[bIndex+1];
        const float denom = (float)std::max<uint64_t>(1, b.recvMs - a.recvMs);
        const float t = clampf((float)(target - a.recvMs) / denom, 0.0f, 1.0f);
        renderSnap = interp(a, b, t);
      } else {
        renderSnap = views.back().snap;
      }

      // Reconcile local player position to latest server snapshot softly (cheap, stable).
      const auto& srv = views.back().snap.players[myIndex];
      Vec2 lp = local.players[myIndex].pos;
      Vec2 d = {srv.pos.x - lp.x, srv.pos.y - lp.y};
      const float err2 = len2(d);
      if (err2 > 6.0f*6.0f) {
        local.players[myIndex].pos = {lp.x + d.x*0.35f, lp.y + d.y*0.35f};
      }

      // Always use local predicted player for immediate feel.
      renderSnap.players[myIndex].pos = local.players[myIndex].pos;
      renderSnap.players[myIndex].aimRad = local.players[myIndex].aimRad;
    }

    rr.begin();
    rr.drawWorld(local.world);
    rr.drawSnapshot(renderSnap, myIndex);
    rr.end();

    // Manual cap to reduce CPU burn; keep low (1-2ms).
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  rr.shutdown();
  return 0;
}
