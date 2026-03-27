#pragma once
#include <enet/enet.h>
#include <array>
#include <optional>
#include <string>
#include "net/NetCommon.h"
#include "net/Protocol.h"
#include "game/Sim.h"

namespace tn {

class Server {
public:
  explicit Server(const NetConfig& cfg);
  ~Server();

  bool start();
  void run();

private:
  NetConfig cfg;
  ENetHost* host{nullptr};
  Sim sim;

  struct ClientSlot {
    ENetPeer* peer{nullptr};
    bool connected{false};
    uint32_t playerIndex{0};
    uint32_t lastInputSeq{0};
    InputCmd lastInput{};
    std::string name;
  };

  std::array<ClientSlot, kMaxPlayers> clients{};
  uint64_t lastTickMs{0};
  uint64_t lastSnapMs{0};

  void handlePacket(ENetPeer* peer, const uint8_t* data, size_t len);
  std::optional<int> findSlot(ENetPeer* peer);
  std::optional<int> allocSlot(ENetPeer* peer);
  void sendWelcome(ENetPeer* peer, uint32_t playerIndex);
  void broadcastSnapshot();
  void resetRound();
  float roundResetTimer{0.0f};
};

} // namespace tn
