#pragma once
#include <enet/enet.h>
#include <deque>
#include <string>
#include <array>
#include "net/NetCommon.h"
#include "net/Protocol.h"
#include "game/Sim.h"

namespace tn {

struct ClientView {
  Snapshot snap{};
  uint64_t recvMs{0};
  std::array<uint32_t, kMaxPlayers> ackSeq{};
};

class Client {
public:
  Client(const NetConfig& cfg, std::string name);
  ~Client();

  bool connect();
  void disconnect();

  // Call each frame.
  void pumpNetwork();
  void sendInput(const InputCmd& in);

  bool hasWelcome() const { return welcomed; }
  uint32_t playerIndex() const { return myIndex; }
  uint32_t playerId() const { return myId; }

  // Latest snapshot for rendering/interp.
  const std::deque<ClientView>& snapshots() const { return views; }
  uint32_t rttMs() const;

private:
  NetConfig cfg;
  std::string name;
  ENetHost* host{nullptr};
  ENetPeer* peer{nullptr};

  bool welcomed{false};
  uint32_t myIndex{0};
  uint32_t myId{0};

  std::deque<ClientView> views; // small buffer for interpolation

  void handlePacket(const uint8_t* data, size_t len);
  void sendHello();
};

} // namespace tn
