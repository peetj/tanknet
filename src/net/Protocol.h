#pragma once
#include <cstdint>

namespace tn {

// Fixed tick rate for sim + net.
constexpr uint32_t kTickHz = 60;

enum class MsgType : uint8_t {
  Hello = 1,        // C->S
  Welcome = 2,      // S->C
  Input = 3,        // C->S
  Snapshot = 4,     // S->C
  Ping = 5,         // both
  Pong = 6          // both
};

// Snapshot includes per-player ack of last processed input sequence.


} // namespace tn
