#include "PipeEnd.h"

namespace Steam
{
void PipeEnd::Close()
{
  if (!m_is_open)
  {
    return;
  }

  CloseImpl();

  m_is_open = false;
  m_handle = INVALID_PIPE_HANDLE;
}

int PipeEnd::Read(void* buffer, const size_t size)
{
  if (!m_is_open)
  {
    return -1;
  }

  return ReadImpl(buffer, size);
}

int PipeEnd::Write(const void* buffer, size_t size)
{
  if (!m_is_open)
  {
    return -1;
  }

  return WriteImpl(buffer, size);
}
}
