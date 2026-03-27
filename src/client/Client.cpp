#include "client/Client.h"
#include "net/Serialize.h"
#include <iostream>

namespace tn {

Client::Client(const NetConfig& c, std::string n) : cfg(c), name(std::move(n)) {}

Client::~Client() {
  disconnect();
  if (host) enet_host_destroy(host);
  host = nullptr;
  enet_deinitialize();
}

bool Client::connect() {
  if (enet_initialize() != 0) {
    std::cerr << "enet init failed\n";
    return false;
  }

  host = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!host) {
    std::cerr << "enet client host create failed\n";
    return false;
  }

  ENetAddress addr{};
  enet_address_set_host(&addr, cfg.host.c_str());
  addr.port = cfg.port;

  peer = enet_host_connect(host, &addr, 2, 0);
  if (!peer) {
    std::cerr << "connect failed\n";
    return false;
  }

  // Wait briefly for connect, then hello.
  ENetEvent ev{};
  if (enet_host_service(host, &ev, 2000) > 0 && ev.type == ENET_EVENT_TYPE_CONNECT) {
    sendHello();
    return true;
  }

  std::cerr << "connect timeout\n";
  return false;
}

void Client::disconnect() {
  if (!peer) return;
  enet_peer_disconnect(peer, 0);
  peer = nullptr;
}

void Client::sendHello() {
  ByteWriter w;
  w.u8((uint8_t)MsgType::Hello);
  auto* pkt = enet_packet_create(w.b.data(), w.b.size(), ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, pkt);
  enet_host_flush(host);
}

void Client::sendInput(const InputCmd& in) {
  if (!peer) return;
  ByteWriter w;
  w.u8((uint8_t)MsgType::Input);
  w.u32(in.seq);
  w.u8(in.moveX);
  w.u8(in.moveY);
  w.i16(in.aimQ);
  w.u8(in.shoot);

  auto* pkt = enet_packet_create(w.b.data(), w.b.size(), 0);
  enet_peer_send(peer, 0, pkt);
}

void Client::handlePacket(const uint8_t* data, size_t len) {
  ByteReader r{std::span<const uint8_t>(data, len)};
  const auto type = (MsgType)r.u8();

  if (type == MsgType::Welcome) {
    myIndex = r.u32();
    myId = r.u32();
    welcomed = true;
    std::cerr << "Welcome. playerIndex=" << myIndex << " id=" << myId << "\n";
    return;
  }

  if (type == MsgType::Snapshot) {
    ClientView v;
    v.recvMs = nowMs();
    v.snap.tick = r.u32();

    for (int i=0;i<kMaxPlayers;i++) {
      auto& p = v.snap.players[i];
      p.id = r.u32();
      p.pos.x = r.f32(); p.pos.y = r.f32();
      p.aimRad = r.f32();
      p.hp = (int)r.u8();
      p.alive = r.u8() != 0;
      v.ackSeq[i] = r.u32();
      v.fireCd[i] = r.f32();
    }

    // Projectiles: clear then fill actives
    for (auto& pr : v.snap.projectiles) pr = {};
    const uint16_t active = r.u16();
    for (uint16_t n=0;n<active;n++) {
      const uint16_t idx = r.u16();
      if (idx >= kMaxProjectiles) break;
      auto& pr = v.snap.projectiles[idx];
      pr.active = true;
      pr.ownerId = r.u32();
      pr.pos.x = r.f32(); pr.pos.y = r.f32();
      pr.vel.x = r.f32(); pr.vel.y = r.f32();
      pr.ttl = r.f32();
    }

    views.push_back(v);
    while (views.size() > 32) views.pop_front();
  }
}

uint32_t Client::rttMs() const {
  if (!peer) return 0;
  return (uint32_t)peer->roundTripTime;
}

void Client::pumpNetwork() {
  if (!host) return;
  ENetEvent ev{};
  while (enet_host_service(host, &ev, 0) > 0) {
    switch (ev.type) {
      case ENET_EVENT_TYPE_RECEIVE:
        handlePacket(ev.packet->data, ev.packet->dataLength);
        enet_packet_destroy(ev.packet);
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        peer = nullptr;
        break;
      default: break;
    }
  }
}

} // namespace tn
