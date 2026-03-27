#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include <cstring>

namespace tn {

struct ByteWriter {
  std::vector<uint8_t> b;
  void u8(uint8_t v) { b.push_back(v); }
  void u16(uint16_t v) { u8((uint8_t)(v & 0xFF)); u8((uint8_t)(v >> 8)); }
  void u32(uint32_t v) { for (int i=0;i<4;i++) u8((uint8_t)((v >> (8*i)) & 0xFF)); }
  void i16(int16_t v) { u16((uint16_t)v); }
  void i32(int32_t v) { u32((uint32_t)v); }
  void f32(float v) { uint32_t u; std::memcpy(&u, &v, 4); u32(u); }
  void bytes(std::span<const uint8_t> s) { b.insert(b.end(), s.begin(), s.end()); }
};

struct ByteReader {
  std::span<const uint8_t> s;
  size_t p{0};
  bool ok{true};
  uint8_t u8() { if (p+1> s.size()) { ok=false; return 0; } return s[p++]; }
  uint16_t u16() { uint16_t a=u8(); uint16_t b=u8(); return (uint16_t)(a | (b<<8)); }
  uint32_t u32() { uint32_t v=0; for(int i=0;i<4;i++) v |= (uint32_t)u8() << (8*i); return v; }
  int16_t i16() { return (int16_t)u16(); }
  int32_t i32() { return (int32_t)u32(); }
  float f32() { uint32_t u=u32(); float f; std::memcpy(&f,&u,4); return f; }
};

} // namespace tn
