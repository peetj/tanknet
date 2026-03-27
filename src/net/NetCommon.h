#pragma once

#include <enet/enet.h>
#include <cstdint>
#include <string>

namespace tn {

struct NetConfig {
  uint16_t port{27015};
  std::string host{"127.0.0.1"};
  uint32_t snapshotHz{20};
  uint32_t inputHz{60};
};

inline uint64_t nowMs() {
  return (uint64_t)enet_time_get();
}

} // namespace tn
