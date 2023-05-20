// Copyright 2023 Dolphin Emulator Project
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

enum class InitResult;

namespace Steam
{
InitResult Init();
void Shutdown();
void FetchUsername();
void SetRichPresence(const std::string& value);
void UpdateRichPresence();
}  // namespace Steam
