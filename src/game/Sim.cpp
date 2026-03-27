#include "game/Sim.h"
#include <algorithm>
#include <cmath>

namespace tn {

static AABB playerBox(const Vec2& p, float r) {
  return {{p.x - r, p.y - r}, {p.x + r, p.y + r}};
}
static AABB projBox(const Vec2& p, float r) {
  return {{p.x - r, p.y - r}, {p.x + r, p.y + r}};
}

void Sim::init() {
  tick = 0;
  world.makeTestMap();
  for (int i=0;i<kMaxPlayers;i++) {
    players[i].id = (uint32_t)(i+1);
    players[i].hp = 3;
    players[i].alive = true;
    fireCd[i] = 0.0f;
  }
  players[0].pos = {world.width*0.15f, world.height*0.5f};
  players[1].pos = {world.width*0.85f, world.height*0.5f};

  for (auto& p : projectiles) p = {};
}

static Vec2 decodeMove(uint8_t mx, uint8_t my) {
  auto f = [](uint8_t v) {
    const int iv = (int)v - 127;
    return clampf((float)iv / 127.0f, -1.0f, 1.0f);
  };
  return {f(mx), f(my)};
}

void Sim::integratePlayer(int i, float dt, const InputCmd& in) {
  auto& pl = players[i];
  if (!pl.alive) return;

  pl.aimRad = (float)in.aimQ / 1000.0f;

  Vec2 mv = decodeMove(in.moveX, in.moveY);
  if (len2(mv) > 1.0f) mv = norm(mv);
  pl.vel = mv * cfg.playerSpeed;

  // Simple axis-separated collision for stability.
  Vec2 next = pl.pos;
  next.x += pl.vel.x * dt;
  if (!world.collides(playerBox(next, cfg.playerRadius))) {
    pl.pos.x = next.x;
  }
  next = pl.pos;
  next.y += pl.vel.y * dt;
  if (!world.collides(playerBox(next, cfg.playerRadius))) {
    pl.pos.y = next.y;
  }
}

void Sim::tryFire(int i, const InputCmd& in) {
  auto& pl = players[i];
  if (!pl.alive) return;
  if (!in.shoot) return;
  if (fireCd[i] > 0.0f) return;

  // Find a slot.
  for (auto& pr : projectiles) {
    if (pr.active) continue;
    pr.active = true;
    pr.ownerId = pl.id;
    pr.pos = pl.pos + norm({std::cos(pl.aimRad), std::sin(pl.aimRad)}) * (cfg.playerRadius + cfg.projRadius + 1.0f);
    Vec2 dir = {std::cos(pl.aimRad), std::sin(pl.aimRad)};
    pr.vel = dir * cfg.projSpeed;
    pr.ttl = cfg.projTTL;
    fireCd[i] = cfg.fireCooldown;
    break;
  }
}

void Sim::integrateProjectiles(float dt) {
  for (auto& pr : projectiles) {
    if (!pr.active) continue;
    pr.ttl -= dt;
    if (pr.ttl <= 0.0f) { pr.active = false; continue; }

    Vec2 next = pr.pos + pr.vel * dt;
    // World collision
    if (world.collides(projBox(next, cfg.projRadius))) { pr.active = false; continue; }
    pr.pos = next;

    // Player hit
    for (int i=0;i<kMaxPlayers;i++) {
      auto& pl = players[i];
      if (!pl.alive) continue;
      if (pl.id == pr.ownerId) continue;
      if (aabbOverlap(playerBox(pl.pos, cfg.playerRadius), projBox(pr.pos, cfg.projRadius))) {
        pr.active = false;
        pl.hp -= 1;
        if (pl.hp <= 0) pl.alive = false;
        break;
      }
    }
  }
}

void Sim::step(float dt, const std::array<InputCmd, kMaxPlayers>& inputs) {
  tick++;
  for (int i=0;i<kMaxPlayers;i++) {
    fireCd[i] = std::max(0.0f, fireCd[i] - dt);
  }

  for (int i=0;i<kMaxPlayers;i++) integratePlayer(i, dt, inputs[i]);
  for (int i=0;i<kMaxPlayers;i++) tryFire(i, inputs[i]);
  integrateProjectiles(dt);
}

Snapshot Sim::makeSnapshot() const {
  Snapshot s;
  s.tick = tick;
  s.players = players;
  s.projectiles = projectiles;
  return s;
}

} // namespace tn
