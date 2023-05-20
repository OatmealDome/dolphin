#include <OptionParser.h>
#include <thread>

#include <steam/steam_api.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "HelperServer.h"

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

  if (args.size() != 1 || args[0] != "SecretStringFromDolphin")
  {
#ifdef _WIN32
    MessageBoxW(
        nullptr,
        L"This application is not meant to be launched directly. Run Dolphin from Steam instead.",
        L"Error", MB_ICONERROR);
#else
    // TODO
#endif

    return 1;
  }

#ifdef _WIN32
  PipeHandle incoming_fd = GetStdHandle(STD_INPUT_HANDLE);
  PipeHandle outgoing_fd = GetStdHandle(STD_OUTPUT_HANDLE);
  #else
  auto args = parser->args();

  if (args.size() != 2)
  {
    parser->print_help();
    return 0;
  }

  PipeHandle incoming_fd = std::stoi(args[0]);
  PipeHandle outgoing_fd = std::stoi(args[1]);
  #endif

  Steam::HelperServer server(incoming_fd, outgoing_fd);

  while (server.IsRunning())
  {
    SteamAPI_RunCallbacks();

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // run at 10 Hz
  }

  SteamAPI_Shutdown();

  return 0;
}
