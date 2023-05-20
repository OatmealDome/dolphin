#include "UICommon/Steam/HelperClient.h"

#ifndef _WIN32
#include <unistd.h>
#include <cstdio>
#endif

#include <utility>

#include "Common/Assert.h"
#include "Common/MsgHandler.h"
#include "Common/StringUtil.h"
#include "Common/CommonFuncs.h"
#include "Common/CommonPaths.h"
#include "Common/FileUtil.h"
#include "Core/ConfigManager.h"

namespace Steam
{
static std::unique_ptr<HelperClient> s_client;

bool Init()
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

  std::wstring cmdline = L"\"" + wpath + L"\" SecretStringFromDolphin";

  STARTUPINFO sinfo{.cb = sizeof(sinfo)};
  sinfo.hStdInput = cts_read;
  sinfo.hStdOutput = stc_write;
  sinfo.dwFlags = STARTF_USESTDHANDLES | STARTF_FORCEOFFFEEDBACK;

  PROCESS_INFORMATION pinfo;

  if (!CreateProcessW(wpath.c_str(), cmdline.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr,
                      &sinfo, &pinfo))
  {
    std::string lastError = Common::GetLastErrorString();
    return false;
  }

  CloseHandle(cts_read);
  CloseHandle(stc_write);
  CloseHandle(pinfo.hThread);
  CloseHandle(pinfo.hProcess);

  s_client = std::make_unique<HelperClient>(stc_read, cts_write);
#else // Unix or Unix-like
  int client_to_server[2];
  int server_to_client[2];

  if (pipe(client_to_server) == -1)
  {
    return false;
  }

  if (pipe(server_to_client) == -1)
  {
    fprintf(stderr, "error: stc pipe failure\n");
    return false;
  }

  pid_t child_pid = fork();

  if (child_pid == -1)
  {
    fprintf(stderr, "error: fork() failed\n");
    return false;
  }

  if (child_pid == 0)  // child
  {
    close(server_to_client[0]);
    close(client_to_server[1]);

    std::string incoming_fd_str = std::to_string(client_to_server[0]);
    std::string outgoing_fd_str = std::to_string(server_to_client[1]);

    execl("SteamHelper", "SteamHelper", incoming_fd_str.c_str(), outgoing_fd_str.c_str(), NULL);
  }
  else
  {
    close(client_to_server[0]);
    close(server_to_client[1]);

    s_client = std::make_unique<HelperClient>(server_to_client[0], client_to_server[1]);
  }
#endif

  auto result = s_client->SendMessageWithReply(MessageType::InitRequest).get();

  if (!result.ipcSuccess)
  {
    return false;
  }

  uint8_t api_init_result;
  result.payload >> api_init_result;

  return static_cast<bool>(api_init_result);
}

void Shutdown()
{
    s_client->SendMessageNoReply(MessageType::ShutdownRequest);

    s_client = nullptr;
}

void FetchUsername()
{
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
} // namespace Steam
