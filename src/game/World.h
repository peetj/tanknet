#pragma once
#include <vector>
#include <cstdint>
#include "game/Math.h"

namespace tn {

struct Obstacle {
  AABB box;
};

struct World {
  float width{1280.0f};
  float height{720.0f};
  std::vector<Obstacle> obstacles;

  void makeTestMap();
  bool collides(const AABB& box) const;
};

} // namespace tn
