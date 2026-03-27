#include "render/Render.h"
#include <cmath>

namespace tn {

bool Renderer::init(const RenderConfig& c) {
  cfg = c;
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return false;

  window = SDL_CreateWindow("TankNet", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            cfg.w, cfg.h, SDL_WINDOW_SHOWN);
  if (!window) return false;

  // Request vsync OFF for minimal latency; we cap manually.
  r = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
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

void Renderer::end() {
  SDL_RenderPresent(r);
}

} // namespace tn
