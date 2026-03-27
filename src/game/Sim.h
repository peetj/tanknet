#pragma once
#include <array>
#include <cstdint>
#include "game/Math.h"
#include "game/World.h"

namespace tn {

constexpr int kMaxPlayers = 2;
constexpr int kMaxProjectiles = 128;

struct PlayerState {
  uint32_t id{0};
  Vec2 pos{0,0};
  Vec2 vel{0,0};
  float aimRad{0.0f};
  int hp{3};
  bool alive{true};
};

struct ProjectileState {
  bool active{false};
  uint32_t ownerId{0};
  Vec2 pos{0,0};
  Vec2 vel{0,0};
  float ttl{0.0f};
};

struct InputCmd {
  uint32_t seq{0};
  uint8_t moveX{127}; // 0..254 where 127 ~ 0
  uint8_t moveY{127};
  int16_t aimQ{0};    // quantized radians * 1000
  uint8_t shoot{0};
};

struct Snapshot {
  uint32_t tick{0};
  std::array<PlayerState, kMaxPlayers> players;
  std::array<ProjectileState, kMaxProjectiles> projectiles;
};

struct SimConfig {
  float playerRadius{14.0f};
  float playerSpeed{260.0f};
  float projRadius{4.0f};
  float projSpeed{520.0f};
  float projTTL{1.25f};
  float fireCooldown{0.25f};
};

class Sim {
public:
  SimConfig cfg;
  World world;

  uint32_t tick{0};
  std::array<PlayerState, kMaxPlayers> players{};
  std::array<ProjectileState, kMaxProjectiles> projectiles{};
  std::array<float, kMaxPlayers> fireCd{};

  void init();
  void step(float dt, const std::array<InputCmd, kMaxPlayers>& inputs);
  Snapshot makeSnapshot() const;

private:
  void integratePlayer(int i, float dt, const InputCmd& in);
  void integrateProjectiles(float dt);
  void tryFire(int i, const InputCmd& in);
};

} // namespace tn
