add_executable(SteamHelper
  HelperServer.cpp
  HelperServer.h
  Main.cpp
)

target_link_libraries(SteamHelper
PRIVATE
  cpp-optparse
  SteamHelperCommon
  SteamworksSDK::SteamworksSDK
)

add_custom_command(TARGET SteamHelper POST_BUILD
  COMMAND cp "${SteamworksSDK_LIBRARY}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
)

if(NOT WIN32)
  set_target_properties(SteamHelper PROPERTIES
    OUTPUT_NAME dolphin-steam-helper
  )
endif()

if(APPLE)
  set_target_properties(SteamHelper PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/SteamHelper.entitlements"
    XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--deep --options=runtime"
  )

  target_link_libraries(SteamHelper
  PRIVATE
    ${COREFOUNDATION_LIBRARY}
  )

  if(MACOS_CODE_SIGNING)
    # Code sign make file builds
    add_custom_command(TARGET SteamHelper POST_BUILD
      COMMAND /usr/bin/codesign -f -s "${MACOS_CODE_SIGNING_IDENTITY}" --deep --options=runtime --entitlements "${CMAKE_CURRENT_SOURCE_DIR}/SteamHelper.entitlements" "$<TARGET_FILE:SteamHelper>")
  endif()
endif()
