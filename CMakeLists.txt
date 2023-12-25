cmake_minimum_required(VERSION 3.13.0)

set ("CMAKE_CXX_STANDARD" 17)

project(REPENTOGONUpdater)

include_directories("${CMAKE_SOURCE_DIR}/libs/rapidjson/include/rapidjson" "${CMAKE_SOURCE_DIR}/libs/miniz-3.0.2")

# libcurl
option(BUILD_TESTING "" OFF)
option(BUILD_CURL_EXE "" OFF)
option(BUILD_SHARED_LIBS "" OFF)
option(BUILD_STATIC_LIBS "" ON)
option(ENABLE_THREADED_RESOLVER "" OFF)
option(CURL_USE_SCHANNEL "" ON)
add_subdirectory("libs/curl")

# Crypto++
option(CRYPTOPP_BUILD_TESTING "" OFF)
add_subdirectory("libs/cryptopp-cmake")

# Updater
set(UPDATER_SRC main.cpp ${CMAKE_SOURCE_DIR}/libs/miniz-3.0.2/miniz.c)
add_executable(REPENTOGONUpdater ${UPDATER_SRC})
target_link_libraries(REPENTOGONUpdater libcurl cryptopp)

# Put libraries into folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(cryptopp curl_uninstall libcurl_object libcurl_static PROPERTIES FOLDER "External Libs")
