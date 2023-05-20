// Copyright 2023 Dolphin Emulator Project
// SPDX-License-Identifier: BSD-3-Clause

#include "UICommon/Steam/HelperClient.h"

#ifndef _WIN32
#include <cstdio>
#include <unistd.h>
#endif

#include <utility>

#include "Common/Assert.h"
#include "Common/CommonFuncs.h"
#include "Common/CommonPaths.h"
#include "Common/FileUtil.h"
#include "Common/MsgHandler.h"
#include "Common/StringUtil.h"

#include "Core/ConfigManager.h"

#include "SteamHelperCommon/Constants.h"
#include "SteamHelperCommon/InitResult.h"

namespace Steam
{
static std::unique_ptr<HelperClient> s_client;

InitResult Init()
{
#ifdef _WIN32
  HANDLE cts_read, cts_write;
  HANDLE stc_read, stc_write;

  ASSERT(CreatePipe(&cts_read, &cts_write, nullptr, 0));
  ASSERT(CreatePipe(&stc_read, &stc_write, nullptr, 0));

  ASSERT(SetHandleInformation(cts_read, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT));
  ASSERT(SetHandleInformation(stc_write, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT));

  const std::string path(File::GetExeDirectory() + DIR_SEP + "SteamHelper.exe");
  const auto wpath = UTF8ToWString(path);

  const std::string cmdline("\"" + path + "\" " + STEAM_HELPER_SECRET_STRING);
  auto wcmdline = UTF8ToWString(cmdline);

  STARTUPINFO sinfo{.cb = sizeof(sinfo)};
  sinfo.hStdInput = cts_read;
  sinfo.hStdOutput = stc_write;
  sinfo.wShowWindow = SW_HIDE;
  sinfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK;

  PROCESS_INFORMATION pinfo;

  if (!CreateProcessW(wpath.c_str(), wcmdline.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr,
                      &sinfo, &pinfo))
  {
    std::string lastError = Common::GetLastErrorString();
    return InitResult::Failure;
  }

  CloseHandle(cts_read);
  CloseHandle(stc_write);
  CloseHandle(pinfo.hThread);
  CloseHandle(pinfo.hProcess);

  s_client = std::make_unique<HelperClient>(stc_read, cts_write);
#else  // Unix or Unix-like
  int client_to_server[2];
  int server_to_client[2];

  ASSERT(pipe(client_to_server) == 0);
  ASSERT(pipe(server_to_client) == 0);

  pid_t child_pid = fork();

  if (child_pid == -1)
  {
    PanicAlertFmt("helper init: fork fail");
    return InitResult::Failure;
  }

  ASSERT(child_pid != -1);

  if (child_pid == 0)  // child
  {
    close(server_to_client[0]);
    close(client_to_server[1]);

    dup2(client_to_server[0], STDIN_FILENO);
    dup2(server_to_client[1], STDOUT_FILENO);

    const std::string path(File::GetExeDirectory() + DIR_SEP + "dolphin-steam-helper");

    execl(path.c_str(), "dolphin-steam-helper", STEAM_HELPER_SECRET_STRING, NULL);
  }
  else
  {
    close(client_to_server[0]);
    close(server_to_client[1]);

    s_client = std::make_unique<HelperClient>(server_to_client[0], client_to_server[1]);
  }
#endif

  if (!s_client->IsRunning())
  {
    PanicAlertFmt("helper init: client not running");
    return InitResult::Failure;
  }

  auto result = s_client->SendMessageWithReply(MessageType::InitRequest).get();

  if (!result.ipcSuccess)
  {
    PanicAlertFmt("helper init: ipc request fail");
    return InitResult::Failure;
  }

  uint8_t init_result_raw;
  result.payload >> init_result_raw;

  return static_cast<InitResult>(init_result_raw);
}

void Shutdown()
{
  if (s_client->IsRunning())
  {
    s_client->SendMessageNoReply(MessageType::ShutdownRequest);
  }

  s_client = nullptr;
}

void FetchUsername()
{
  if (!s_client->IsRunning())
  {
    return;
  }

  auto result = s_client->SendMessageWithReply(MessageType::FetchUsernameRequest).get();

  if (!result.ipcSuccess)
  {
    return;
  }

  std::string username;
  result.payload >> username;

  PanicAlertFmt("username: {}", username);
}

void SetRichPresence(const std::string& key, const std::string& value)
{
  if (!s_client->IsRunning())
  {
    return;
  }

  sf::Packet payload;
  payload << key;
  payload << value;

  s_client->SendMessageWithReply(MessageType::SetRichPresenceRequest, payload).get();
}

void UpdateRichPresence()
{
  const std::string& title = SConfig::GetInstance().GetTitleDescription();

  SetRichPresence("steam_display", "#Status_Playing");
  SetRichPresence("CurrentGame", title);
}
}  // namespace Steam
