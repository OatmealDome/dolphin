#pragma once


#include <unordered_map>
#include <future>

#include "SteamHelperCommon/MessageType.h"
#include "SteamHelperCommon/IpcConnection.h"

namespace Steam
{
class HelperClient : IpcConnection
{
public:
  HelperClient(PipeHandle in_handle, PipeHandle out_handle)
    : IpcConnection(in_handle, out_handle) {}

  std::future<sf::Packet> SendRequest(const sf::Packet* data_packet, MessageType message_type);
  void SendRequestNoReply(MessageType type, const sf::Packet* data_packet);

private:
  virtual void Receive(sf::Packet &packet) override;

  std::unordered_map<uint32_t, std::shared_ptr<std::promise<sf::Packet>>> m_promises;
  std::mutex m_promises_mutex;
  uint32_t m_last_call_id = 0;
};
} // namespace Steam
