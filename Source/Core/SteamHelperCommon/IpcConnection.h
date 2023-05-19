// Copyright 2023 Dolphin Emulator Project
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <thread>

#include <SFML/Network/Packet.hpp>

#include "PipeEnd.h"

namespace Steam
{
class IpcConnection
{
public:
  IpcConnection(PipeHandle in_handle, PipeHandle out_handle);
  virtual ~IpcConnection();

  bool IsRunning();

protected:
  void Send(sf::Packet& packet);
  virtual void Receive(sf::Packet& packet) = 0;

private:
  void ReceiveFunc();

  void RequestStop();

  PipeEnd m_incoming_pipe;
  PipeEnd m_outgoing_pipe;

  std::mutex m_send_mutex;

  std::thread m_receive_thread;
  std::atomic_bool m_stop_receiving;

  std::atomic_bool m_is_running;
};
}  // namespace Steam
