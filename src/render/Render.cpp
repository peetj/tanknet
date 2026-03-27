#include "render/Render.h"
#include <cmath>

namespace tn {

bool Renderer::init(const RenderConfig& c, bool vsync) {
  cfg = c;
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return false;

  window = SDL_CreateWindow("TankNet", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            cfg.w, cfg.h, SDL_WINDOW_SHOWN);
  if (!window) return false;

  // Vsync OFF by default for minimal latency.
  uint32_t flags = SDL_RENDERER_ACCELERATED | (vsync ? SDL_RENDERER_PRESENTVSYNC : 0);
  r = SDL_CreateRenderer(window, -1, flags);
  if (!r) return false;

  SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1");
  return true;
}

void Renderer::shutdown() {
  if (r) SDL_DestroyRenderer(r);
  if (window) SDL_DestroyWindow(window);
  r = nullptr; window = nullptr;
  SDL_Quit();
}

void Renderer::begin() {
  SDL_SetRenderDrawColor(r, 14, 16, 18, 255);
  SDL_RenderClear(r);
}

void Renderer::drawWorld(const World& world) {
  SDL_SetRenderDrawColor(r, 70, 76, 82, 255);
  for (const auto& o : world.obstacles) {
    SDL_Rect rc;
    rc.x = (int)o.box.min.x;
    rc.y = (int)o.box.min.y;
    rc.w = (int)(o.box.max.x - o.box.min.x);
    rc.h = (int)(o.box.max.y - o.box.min.y);
    SDL_RenderFillRect(r, &rc);
  }
}

void Renderer::drawCircle(int cx, int cy, int rad) {
  // Cheap filled circle using horizontal lines.
  for (int y = -rad; y <= rad; y++) {
    const int dx = (int)std::sqrt((double)rad*rad - (double)y*y);
    SDL_RenderDrawLine(r, cx - dx, cy + y, cx + dx, cy + y);
  }
}

void Renderer::drawSnapshot(const Snapshot& s, uint32_t myPlayerIndex) {
  // Players
  for (int i=0;i<kMaxPlayers;i++) {
    const auto& p = s.players[i];
    if (!p.alive) continue;

    if ((uint32_t)i == myPlayerIndex) SDL_SetRenderDrawColor(r, 80, 200, 120, 255);
    else SDL_SetRenderDrawColor(r, 220, 90, 90, 255);

    drawCircle((int)p.pos.x, (int)p.pos.y, 14);

    // Aim line
    SDL_SetRenderDrawColor(r, 220, 220, 220, 255);
    const int x2 = (int)(p.pos.x + std::cos(p.aimRad) * 22.0f);
    const int y2 = (int)(p.pos.y + std::sin(p.aimRad) * 22.0f);
    SDL_RenderDrawLine(r, (int)p.pos.x, (int)p.pos.y, x2, y2);
  }

  // Projectiles
  SDL_SetRenderDrawColor(r, 240, 220, 60, 255);
  for (int i=0;i<kMaxProjectiles;i++) {
    const auto& pr = s.projectiles[i];
    if (!pr.active) continue;
    drawCircle((int)pr.pos.x, (int)pr.pos.y, 4);
  }
}

static void drawSeg(SDL_Renderer* r, int x, int y, int w, int h) {
  SDL_Rect rc{x,y,w,h};
  SDL_RenderFillRect(r, &rc);
}

// 7-seg digit (a,b,c,d,e,f,g)
static void drawDigit7(SDL_Renderer* r, int x, int y, int s, int d) {
  const int t = std::max(1, s/4);
  const int w = s;
  const int h = s*2;
  const bool seg[10][7] = {
    {1,1,1,1,1,1,0}, //0
    {0,1,1,0,0,0,0}, //1
    {1,1,0,1,1,0,1}, //2
    {1,1,1,1,0,0,1}, //3
    {0,1,1,0,0,1,1}, //4
    {1,0,1,1,0,1,1}, //5
    {1,0,1,1,1,1,1}, //6
    {1,1,1,0,0,0,0}, //7
    {1,1,1,1,1,1,1}, //8
    {1,1,1,1,0,1,1}  //9
  };
  if (d < 0 || d > 9) return;

  // a
  if (seg[d][0]) drawSeg(r, x + t, y, w - 2*t, t);
  // b
  if (seg[d][1]) drawSeg(r, x + w - t, y + t, t, h/2 - t);
  // c
  if (seg[d][2]) drawSeg(r, x + w - t, y + h/2, t, h/2 - t);
  // d
  if (seg[d][3]) drawSeg(r, x + t, y + h - t, w - 2*t, t);
  // e
  if (seg[d][4]) drawSeg(r, x, y + h/2, t, h/2 - t);
  // f
  if (seg[d][5]) drawSeg(r, x, y + t, t, h/2 - t);
  // g
  if (seg[d][6]) drawSeg(r, x + t, y + h/2 - t/2, w - 2*t, t);
}

static void drawNumber(SDL_Renderer* r, int x, int y, int s, int n) {
  if (n == 0) { drawDigit7(r, x, y, s, 0); return; }
  int digits[10];
  int c = 0;
  int nn = n;
  while (nn > 0 && c < 10) { digits[c++] = nn % 10; nn /= 10; }
  for (int i=c-1;i>=0;i--) {
    drawDigit7(r, x, y, s, digits[i]);
    x += s + s/2;
  }
}

void Renderer::drawHud(int hp0, int hp1, uint32_t rttMs, int resetSec) {
  // Minimal HUD: HP left/right + RTT top-right + reset countdown center-top.
  SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 120);
  SDL_Rect bg{12, 12, 240, 54};
  SDL_RenderFillRect(r, &bg);

  // HP: green vs red
  SDL_SetRenderDrawColor(r, 80, 200, 120, 255);
  drawNumber(r, 20, 20, 14, std::max(0, hp0));
  SDL_SetRenderDrawColor(r, 220, 90, 90, 255);
  drawNumber(r, 90, 20, 14, std::max(0, hp1));

  // RTT
  SDL_SetRenderDrawColor(r, 220, 220, 220, 255);
  SDL_Rect bg2{cfg.w - 160, 12, 148, 54};
  SDL_SetRenderDrawColor(r, 0, 0, 0, 120);
  SDL_RenderFillRect(r, &bg2);
  SDL_SetRenderDrawColor(r, 220, 220, 220, 255);
  drawNumber(r, cfg.w - 150, 20, 14, (int)rttMs);

  // Reset countdown (if any)
  if (resetSec > 0) {
    SDL_Rect bg3{cfg.w/2 - 90, 12, 180, 54};
    SDL_SetRenderDrawColor(r, 0, 0, 0, 120);
    SDL_RenderFillRect(r, &bg3);
    SDL_SetRenderDrawColor(r, 240, 220, 60, 255);
    drawNumber(r, cfg.w/2 - 20, 20, 14, resetSec);
  }
}

void Renderer::end() {
  SDL_RenderPresent(r);
}

} // namespace tn
