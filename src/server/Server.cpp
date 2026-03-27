#include "server/Server.h"
#include "net/Serialize.h"
#include <iostream>
#include <thread>

namespace tn {

Server::Server(const NetConfig& c) : cfg(c) {}

Server::~Server() {
  if (host) enet_host_destroy(host);
  host = nullptr;
  enet_deinitialize();
}

bool Server::start() {
  if (enet_initialize() != 0) {
    std::cerr << "enet init failed\n";
    return false;
  }

  ENetAddress addr{};
  addr.host = ENET_HOST_ANY;
  addr.port = cfg.port;

  host = enet_host_create(&addr, /*peers*/ 8, /*channels*/ 2, /*inBW*/ 0, /*outBW*/ 0);
  if (!host) {
    std::cerr << "enet host create failed\n";
    return false;
  }

  sim.init();
  lastTickMs = nowMs();
  lastSnapMs = nowMs();
  std::cout << "Server listening on port " << cfg.port << "\n";
  return true;
}

std::optional<int> Server::findSlot(ENetPeer* peer) {
  for (int i=0;i<kMaxPlayers;i++) if (clients[i].peer == peer) return i;
  return std::nullopt;
}

std::optional<int> Server::allocSlot(ENetPeer* peer) {
  for (int i=0;i<kMaxPlayers;i++) {
    if (!clients[i].connected) {
      clients[i] = {};
      clients[i].peer = peer;
      clients[i].connected = true;
      clients[i].playerIndex = (uint32_t)i;
      clients[i].lastInput = {};
      return i;
    }
  }
  return std::nullopt;
}

void Server::sendWelcome(ENetPeer* peer, uint32_t playerIndex) {
  ByteWriter w;
  w.u8((uint8_t)MsgType::Welcome);
  w.u32(playerIndex);
  w.u32(sim.players[playerIndex].id);
  auto* pkt = enet_packet_create(w.b.data(), w.b.size(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, pkt);
}

void Server::broadcastSnapshot() {
  const Snapshot s = sim.makeSnapshot();
  ByteWriter w;
  w.u8((uint8_t)MsgType::Snapshot);
  w.u32(s.tick);

  // Players
  for (int i=0;i<kMaxPlayers;i++) {
    const auto& p = s.players[i];
    w.u32(p.id);
    w.f32(p.pos.x); w.f32(p.pos.y);
    w.f32(p.aimRad);
    w.u8((uint8_t)p.hp);
    w.u8((uint8_t)(p.alive ? 1 : 0));
  }

  // Projectiles (simple full state, still small)
  w.u16((uint16_t)kMaxProjectiles);
  for (int i=0;i<kMaxProjectiles;i++) {
    const auto& pr = s.projectiles[i];
    w.u8(pr.active ? 1 : 0);
    if (pr.active) {
      w.u32(pr.ownerId);
      w.f32(pr.pos.x); w.f32(pr.pos.y);
      w.f32(pr.vel.x); w.f32(pr.vel.y);
      w.f32(pr.ttl);
    }
  }

  auto* pkt = enet_packet_create(w.b.data(), w.b.size(), 0);
  enet_host_broadcast(host, 0, pkt);
}

void Server::handlePacket(ENetPeer* peer, const uint8_t* data, size_t len) {
  ByteReader r{std::span<const uint8_t>(data, len)};
  const auto type = (MsgType)r.u8();

  if (type == MsgType::Hello) {
    auto slot = findSlot(peer);
    if (!slot) slot = allocSlot(peer);
    if (!slot) {
      std::cerr << "server full\n";
      enet_peer_disconnect(peer, 0);
      return;
    }
    // name (optional, not sent yet) - keep placeholder.
    sendWelcome(peer, (uint32_t)*slot);
    return;
  }

  auto slot = findSlot(peer);
  if (!slot) return;

  if (type == MsgType::Input) {
    InputCmd in{};
    in.seq = r.u32();
    in.moveX = r.u8();
    in.moveY = r.u8();
    in.aimQ = r.i16();
    in.shoot = r.u8();

    auto& c = clients[*slot];
    if (in.seq >= c.lastInputSeq) {
      c.lastInputSeq = in.seq;
      c.lastInput = in;
    }
  }
}

void Server::run() {
  constexpr float dt = 1.0f / (float)kTickHz;

  while (true) {
    // Pump network
    ENetEvent ev{};
    while (enet_host_service(host, &ev, 0) > 0) {
      switch (ev.type) {
        case ENET_EVENT_TYPE_CONNECT:
          // wait for Hello
          break;
        case ENET_EVENT_TYPE_RECEIVE:
          handlePacket(ev.peer, ev.packet->data, ev.packet->dataLength);
          enet_packet_destroy(ev.packet);
          break;
        case ENET_EVENT_TYPE_DISCONNECT: {
          auto slot = findSlot(ev.peer);
          if (slot) clients[*slot] = {};
          break;
        }
        default: break;
      }
    }

    // Fixed tick
    const uint64_t ms = nowMs();
    if (ms - lastTickMs >= (1000 / kTickHz)) {
      lastTickMs += (1000 / kTickHz);

      std::array<InputCmd, kMaxPlayers> ins{};
      for (int i=0;i<kMaxPlayers;i++) {
        ins[i] = clients[i].connected ? clients[i].lastInput : InputCmd{};
      }
      sim.step(dt, ins);
    }

    // Snapshots
    const uint64_t snapPeriodMs = 1000 / std::max(1u, cfg.snapshotHz);
    if (ms - lastSnapMs >= snapPeriodMs) {
      lastSnapMs = ms;
      broadcastSnapshot();
      enet_host_flush(host);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

} // namespace tn
