#include "PipeEnd.h"

namespace Steam
{
void PipeEnd::Close()
{
  std::scoped_lock lock(m_mutex);

  if (!m_is_open)
  {
    return;
  }

  CloseImpl();

  m_is_open = false;
}

int PipeEnd::Read(void* buffer, const size_t size)
{
  std::scoped_lock lock(m_mutex);

  if (!m_is_open)
  {
    return -1;
  }

  return ReadImpl(buffer, size);
}

int PipeEnd::Write(const void* buffer, size_t size)
{
  std::scoped_lock lock(m_mutex);

  if (!m_is_open)
  {
    return -1;
  }

  return WriteImpl(buffer, size);
}
}
