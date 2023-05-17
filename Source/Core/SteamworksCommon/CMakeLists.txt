add_library(SteamworksCommon
  RpcConnection.h
  RpcConnection.cpp
)

target_link_libraries(SteamworksCommon
PUBLIC
  sfml-network
  sfml-system
)
