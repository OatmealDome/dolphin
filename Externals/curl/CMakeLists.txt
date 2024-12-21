option(BUILD_CURL_EXE "" OFF)
option(BUILD_SHARED_LIBS "" OFF)
option(BUILD_STATIC_LIBS "" ON)
option(BUILD_TESTS "" OFF)
option(BUILD_EXAMPLES "" OFF)
option(USE_LIBIDN2 "" OFF)

set(ZLIB_FOUND OFF)

add_subdirectory(curl)
