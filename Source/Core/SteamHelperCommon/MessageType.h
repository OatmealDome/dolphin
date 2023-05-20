#pragma once

#include <cstdint>

namespace Steam
{
enum class MessageType : uint8_t
{
  Invalid = 0,
  InitRequest,
  InitReply,
  FetchUsernameRequest,
  FetchUsernameReply,
  SetRichPresenceRequest,
  SetRichPresenceReply,
  ShutdownRequest
};
}
