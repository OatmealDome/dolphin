// Copyright 2023 Dolphin Emulator Project
// SPDX-License-Identifier: BSD-3-Clause

#include "IpcConnection.h"

#include <cassert>

#define MAX_PACKET_SIZE 0x100

namespace Steam
{
IpcConnection::IpcConnection(PipeHandle in_handle, PipeHandle out_handle)
  : m_in_end(PipeEnd(in_handle)), m_out_end(PipeEnd(out_handle))
{
  m_is_running.store(true);

  m_receive_thread = std::thread(&IpcConnection::ReceiveThreadFunc, this);
}

IpcConnection::~IpcConnection()
{
  RequestStop();

  m_in_end.Close();
  m_out_end.Close();

  m_receive_thread.join();
}

bool IpcConnection::IsRunning()
{
  return m_is_running.load();
}

void IpcConnection::RequestStop()
{
  m_is_running.store(false);
}

void IpcConnection::Send(sf::Packet& packet)
{
  std::scoped_lock lock(m_out_mutex);

  if (!m_is_running.load())
  {
    return;
  }

  const size_t total_size = sizeof(std::size_t) + packet.getDataSize();
  assert(total_size <= MAX_PACKET_SIZE);

  std::size_t size = packet.getDataSize();
  if (m_out_end.Write(&size, sizeof(std::size_t)) < 0)
  {
    RequestStop();
    return;
  }

  if (m_out_end.Write(packet.getData(), packet.getDataSize()) < 0)
  {
    RequestStop();
    return;
  }
}

void IpcConnection::ReceiveThreadFunc()
{
  while (m_is_running.load())
  {
    sf::Packet packet;

    std::size_t size;

    int result = m_in_end.Read(&size, sizeof(std::size_t));
    if (result <= 0) // EOF or error
    {
      break;
    }

    uint8_t buffer[MAX_PACKET_SIZE];

    result = m_in_end.Read(&buffer, size);
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
};
