#pragma once
#include <cstdint>
#include <cmath>

namespace tn {

struct Vec2 {
  float x{0}, y{0};
  constexpr Vec2() = default;
  constexpr Vec2(float X, float Y) : x(X), y(Y) {}
  constexpr Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
  constexpr Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
  constexpr Vec2 operator*(float s) const { return {x * s, y * s}; }
  constexpr Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
};

inline float dot(const Vec2& a, const Vec2& b) { return a.x*b.x + a.y*b.y; }
inline float len2(const Vec2& v) { return dot(v,v); }
inline float len(const Vec2& v) { return std::sqrt(len2(v)); }
inline Vec2 norm(const Vec2& v) {
  const float l = len(v);
  if (l <= 1e-6f) return {0,0};
  return {v.x / l, v.y / l};
}
inline float clampf(float v, float lo, float hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

struct AABB {
  Vec2 min, max;
};

inline bool aabbOverlap(const AABB& a, const AABB& b) {
  return (a.min.x <= b.max.x && a.max.x >= b.min.x &&
          a.min.y <= b.max.y && a.max.y >= b.min.y);
}

} // namespace tn
