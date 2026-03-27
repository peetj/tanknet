#include "game/World.h"

namespace tn {

void World::makeTestMap() {
  obstacles.clear();
  // Border walls (thick)
  const float t = 24.0f;
  obstacles.push_back({{{0,0},{width,t}}});
  obstacles.push_back({{{0,height-t},{width,height}}});
  obstacles.push_back({{{0,0},{t,height}}});
  obstacles.push_back({{{width-t,0},{width,height}}});

  // Interior obstacles
  obstacles.push_back({{{width*0.25f, height*0.25f}, {width*0.35f, height*0.75f}}});
  obstacles.push_back({{{width*0.65f, height*0.25f}, {width*0.75f, height*0.75f}}});
  obstacles.push_back({{{width*0.45f, height*0.45f}, {width*0.55f, height*0.55f}}});
}

bool World::collides(const AABB& box) const {
  for (const auto& o : obstacles) {
    if (aabbOverlap(box, o.box)) return true;
  }
  return false;
}

} // namespace tn
