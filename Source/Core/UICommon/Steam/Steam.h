// Copyright 2023 Dolphin Emulator Project
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

namespace Steam
{
bool Init();
void Shutdown();
void FetchUsername();
void SetRichPresence(const std::string& value);
void UpdateRichPresence();
}  // namespace Steam
