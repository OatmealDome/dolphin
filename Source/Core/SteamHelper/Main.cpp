#include <OptionParser.h>
#include <thread>

#include <steam/steam_api.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "SteamHelperCommon/Constants.h"

#include "SteamHelper/HelperServer.h"

int main(int argc, char* argv[])
{
  fprintf(stderr, "Hello, world!\n");

  auto parser = std::make_unique<optparse::OptionParser>();
  parser->usage("usage: %prog <secret>");
  parser->parse_args(argc, argv);

  auto args = parser->args();

  for (int i = 0; i < args.size(); i++)
  {
    fprintf(stderr, "arg %d: %s\n", i, args[0].c_str());
  }

  if (args.size() != 1 || args[0] != STEAM_HELPER_SECRET_STRING)
  {
#ifdef _WIN32
    MessageBoxW(
        nullptr,
        L"This application is not meant to be launched directly. Run Dolphin from Steam instead.",
        L"Error", MB_ICONERROR);
#else
    // TODO
    fprintf(stderr, "fatal\n");
#endif

    return 1;
  }

#ifdef _WIN32
  PipeHandle in_handle = GetStdHandle(STD_INPUT_HANDLE);
  PipeHandle out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
#else
  PipeHandle in_handle = STDIN_FILENO;
  PipeHandle out_handle = STDOUT_FILENO;
#endif

  Steam::HelperServer server(in_handle, out_handle);

  while (server.IsRunning())
  {
    SteamAPI_RunCallbacks();

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // run at 10 Hz
  }

  SteamAPI_Shutdown();

  return 0;
}
