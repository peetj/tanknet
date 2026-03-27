#pragma once
#include <SDL.h>
#include "game/Sim.h"
#include "game/World.h"

namespace tn {

struct RenderConfig {
  int w{1280};
  int h{720};
};

class Renderer {
public:
  bool init(const RenderConfig& cfg);
  void shutdown();

  void begin();
  void drawWorld(const World& world);
  void drawSnapshot(const Snapshot& s, uint32_t myPlayerIndex);
  void end();

  SDL_Window* window{nullptr};
  SDL_Renderer* r{nullptr};

private:
  RenderConfig cfg;
  void drawCircle(int cx, int cy, int rad);
};

} // namespace tn
