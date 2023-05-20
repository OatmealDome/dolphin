// Copyright 2023 Dolphin Emulator Project
// SPDX-License-Identifier: BSD-3-Clause

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

  void ReceiveInitRequest(uint32_t call_id);
  void ReceiveFetchUsernameRequest(uint32_t call_id);
  void ReceiveSetRichPresenceRequest(uint32_t call_id, const std::string& key, const std::string& value);
  void ReceiveShutdownRequest();
};
}  // namespace Steam
