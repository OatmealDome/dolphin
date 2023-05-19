// Copyright 2023 Dolphin Emulator Project
// SPDX-License-Identifier: BSD-3-Clause

#include "IpcConnection.h"

#include <cassert>

#define MAX_PACKET_SIZE 0x100

namespace Steam
{
IpcConnection::IpcConnection(PipeHandle in_handle, PipeHandle out_handle)
  : m_incoming_pipe(PipeEnd(in_handle)), m_outgoing_pipe(PipeEnd(out_handle))
{
  m_stop_receiving.store(false);
  m_is_running.store(true);

  m_receive_thread = std::thread(&IpcConnection::ReceiveFunc, this);
}

IpcConnection::~IpcConnection()
{
  RequestStop();

  m_receive_thread.join();
}

bool IpcConnection::IsRunning()
{
  return m_is_running.load();
}

void IpcConnection::RequestStop()
{
  m_stop_receiving.store(true);

  m_outgoing_pipe.Close();
  m_incoming_pipe.Close();
}

void IpcConnection::Send(sf::Packet& packet)
{
  std::scoped_lock lock(m_send_mutex);

  const size_t total_size = sizeof(std::size_t) + packet.getDataSize();
  assert(total_size <= MAX_PACKET_SIZE);

  std::size_t size = packet.getDataSize();
  if (m_outgoing_pipe.Write(&size, sizeof(std::size_t)) < 0)
  {
    return;
  }

  if (m_outgoing_pipe.Write(packet.getData(), packet.getDataSize()) < 0)
  {
    return;
  }
}

void IpcConnection::ReceiveFunc()
{
  while (!m_stop_receiving.load())
  {
    sf::Packet packet;

    std::size_t size;

    int result = m_incoming_pipe.Read(&size, sizeof(std::size_t));
    if (result <= 0) // EOF or error
    {
      break;
    }

    uint8_t buffer[MAX_PACKET_SIZE];

    result = m_incoming_pipe.Read(&buffer, size);
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
