cmake_minimum_required(VERSION 3.13.0)

set ("CMAKE_CXX_STANDARD" 17)

project(REPENTOGONUpdater)

find_package( OpenGL REQUIRED )

include_directories("${CMAKE_SOURCE_DIR}/libs/rapidjson/include/rapidjson" "${CMAKE_SOURCE_DIR}/libs/miniz-3.0.2" 
"${CMAKE_SOURCE_DIR}/libs/imgui" "${CMAKE_SOURCE_DIR}/libs/imgui/backends" ${OPENGL_INCLUDE_DIRS})

# libcurl
option(BUILD_TESTING "" OFF)
option(BUILD_CURL_EXE "" OFF)
option(BUILD_SHARED_LIBS "" OFF)
option(BUILD_STATIC_LIBS "" ON)
option(ENABLE_THREADED_RESOLVER "" OFF)
if(LINUX) 
	option(CURL_USE_OPENSSL "" ON)
else()
	option(CURL_USE_SCHANNEL "" ON)
endif()

# Disable everything except HTTPS, we don't need to compile most of this.
option(CURL_DISABLE_DICT "" ON)
option(CURL_DISABLE_FILE "" ON)
option(CURL_DISABLE_FTP "" ON)
option(CURL_DISABLE_GOPHER "" ON)
option(CURL_DISABLE_IMAP "" ON)
option(CURL_DISABLE_LDAP "" ON)
option(CURL_DISABLE_LDAPS "" ON)
option(CURL_DISABLE_MQTT "" ON)
option(CURL_DISABLE_POP3 "" ON)
option(CURL_DISABLE_RTSP "" ON)
option(CURL_DISABLE_SMB "" ON)
option(CURL_DISABLE_SMTP "" ON)
option(CURL_DISABLE_TELNET "" ON)
option(CURL_DISABLE_TFTP "" ON)
add_subdirectory("libs/curl")

# Crypto++
option(CRYPTOPP_BUILD_TESTING "" OFF)
add_subdirectory("libs/cryptopp-cmake")

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("libs/glfw")
link_libraries(${GLFW_LIBRARY_DIRS})

if( MSVC )
    SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup" )
endif()

# Dear ImGui
FILE(GLOB IMGUI_SRC ${CMAKE_SOURCE_DIR}/libs/imgui/*.cpp)
FILE(GLOB IMGUI_BACKENDS_DIR ${CMAKE_SOURCE_DIR}/libs/imgui/backends/)
set(IMGUI_BACKENDS_DIR ${CMAKE_SOURCE_DIR}/libs/imgui/backends/)
add_library(imgui ${IMGUI_SRC} ${IMGUI_BACKENDS_DIR}/imgui_impl_opengl3.cpp ${IMGUI_BACKENDS_DIR}/imgui_impl_opengl3.h ${IMGUI_BACKENDS_DIR}/imgui_impl_opengl3_loader.h ${IMGUI_BACKENDS_DIR}/imgui_impl_glfw.h ${IMGUI_BACKENDS_DIR}/imgui_impl_glfw.cpp)
target_link_libraries(imgui glfw ${OPENGL_LIBRARY})

# Updater
set(UPDATER_SRC ${CMAKE_SOURCE_DIR}/src/main.cpp ${CMAKE_SOURCE_DIR}/src/LogViewer.h ${CMAKE_SOURCE_DIR}/libs/miniz-3.0.2/miniz.c)
add_executable(REPENTOGONUpdater ${UPDATER_SRC})
target_link_libraries(REPENTOGONUpdater libcurl cryptopp imgui glfw ${OPENGL_LIBRARY})
