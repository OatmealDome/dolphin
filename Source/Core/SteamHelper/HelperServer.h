#pragma once

#include "SteamHelperCommon/IpcConnection.h"

namespace Steam
{
class HelperServer : public IpcConnection
{
public:
  HelperServer(PipeHandle in_handle, PipeHandle out_handle)
    : IpcConnection(in_handle, out_handle) {}

private:
  virtual void Receive(sf::Packet &packet) override;

  void ReceiveTestRequest(uint32_t call_id);
  void ReceiveInitRequest(uint32_t call_id);
  void ReceiveFetchUsernameRequest(uint32_t call_id);
  void ReceiveSetRichPresenceRequest(uint32_t call_id, const std::string& key, const std::string& value);
};
} // namespace Steam
