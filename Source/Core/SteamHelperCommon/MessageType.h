#pragma once

#include <cstdint>

namespace Steam
{
enum class MessageType : uint8_t
{
    TestRequest = 0,
    TestReply = 1,
    InitRequest = 2,
    InitReply = 3,
    FetchUsernameRequest = 4,
    FetchUsernameReply = 5,
    SetRichPresenceRequest = 6,
    SetRichPresenceReply = 7,
};
}
