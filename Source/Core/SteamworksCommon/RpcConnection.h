// Copyright 2023 Dolphin Emulator Project
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <thread>

#include <SFML/Network/Packet.hpp>

class RpcConnection
{
public:
  RpcConnection(int incoming_fd, int outgoing_fd);
  ~RpcConnection();

  bool IsRunning();

protected:
  void Send(sf::Packet& packet);
  virtual void Receive(sf::Packet& packet) = 0;

private:
  void ReceiveFunc();

  void RequestStop();

  int m_incoming_fd;
  int m_outgoing_fd;

  std::mutex m_send_mutex;

  std::thread m_receive_thread;
  std::atomic_bool m_stop_receiving;

  std::atomic_bool m_is_running;
};

