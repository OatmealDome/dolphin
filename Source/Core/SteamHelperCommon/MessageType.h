// Copyright 2023 Dolphin Emulator Project
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

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
}  // namespace Steam
