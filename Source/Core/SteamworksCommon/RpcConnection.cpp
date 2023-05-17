// Copyright 2023 Dolphin Emulator Project
// SPDX-License-Identifier: BSD-3-Clause

#include "RpcConnection.h"

#include <unistd.h>

RpcConnection::RpcConnection(int incoming_fd, int outgoing_fd) : m_incoming_fd(incoming_fd), m_outgoing_fd(outgoing_fd)
{
  m_stop_receiving.store(false);
  m_is_running.store(true);

  m_receive_thread = std::thread(&RpcConnection::ReceiveFunc, this);
}

RpcConnection::~RpcConnection()
{
  RequestStop();

  m_receive_thread.join();
}

bool RpcConnection::IsRunning()
{
  return m_is_running.load();
}

void RpcConnection::RequestStop()
{
  m_stop_receiving.store(true);

  close(m_outgoing_fd);
  m_outgoing_fd = -1;

  close(m_incoming_fd);
  m_incoming_fd = -1;
}

void RpcConnection::Send(sf::Packet& packet)
{
  std::scoped_lock lock(m_send_mutex);

  std::size_t size = packet.getDataSize();
  if (write(m_outgoing_fd, &size, sizeof(std::size_t)) < 0)
  {
    return;
  }

  if (write(m_outgoing_fd, packet.getData(), packet.getDataSize()) < 0)
  {
    return;
  }
}

void RpcConnection::ReceiveFunc()
{
  while (!m_stop_receiving.load())
  {
    sf::Packet packet;

    std::size_t size;

    int result = read(m_incoming_fd, &size, sizeof(std::size_t));
    if (result <= 0) // EOF or error
    {
      break;
    }

    uint8_t buffer[size];

    result = read(m_incoming_fd, &buffer, size);
    if (result <= 0)
    {
      break;
    }

    packet.append(&buffer, size);

    Receive(packet);
  }

  m_is_running.store(false);

  RequestStop();
}
