# Copyright (C) 2022. Baulk contributors. All Rights Reserved.
cmake_minimum_required(VERSION 3.22)
project(WinMenu)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE)
  message(
    FATAL_ERROR
      "In-source builds are not allowed.
CMake would overwrite the makefiles distributed with Baulk.
Please create a directory and run cmake from there, passing the path
to this source directory as the last argument.
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
Please delete them.")
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(WINMENU_ENABLE_LTO OFF)
include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT error)
if(lto_supported)
  message(STATUS "IPO/LTO supported")
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(WINMENU_ENABLE_LTO ON)
    set(BELA_ENABLE_LTO ON)
    message(STATUS "IPO/LTO enabled")
  endif()
endif()

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY_DEFAULT "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS=1 -utf-8 -W3 -DUNICODE=1 -D_UNICODE=1 -wd26812")
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS=1 -utf-8 -W3 -permissive- -Zc:__cplusplus -DUNICODE=1 -D_UNICODE=1 -wd26812"
  )
  set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -c65001")
endif(MSVC)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
option(BUILD_TEST "build test" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
# Gen version
include(VersionFromVCS)
get_source_info(${CMAKE_CURRENT_SOURCE_DIR} WINMENU_REVISION WINMENU_REMOTE_URL WINMENU_REFNAME)
string(TIMESTAMP WINMENU_COPYRIGHT_YEAR "%Y")

# replace to GITHUB_REF
if(DEFINED ENV{GITHUB_REF})
  set(WINMENU_REFNAME $ENV{GITHUB_REF})
endif()

message(STATUS "WINMENU_REFNAME: ${WINMENU_REFNAME}")

if("$ENV{GITHUB_REF_TYPE}" MATCHES "tag")
  string(TIMESTAMP WINMENU_BUILD_TIME "%Y-%m-%dT%H:%M:%SZ")
else()
  set(WINMENU_BUILD_TIME "none")
endif()

message(STATUS "baulk build time: ${WINMENU_BUILD_TIME}")

if(DEFINED ENV{GITHUB_RUN_NUMBER})
  set(WINMENU_VERSION_BUILD $ENV{GITHUB_RUN_NUMBER})
  message(STATUS "baulk detect GITHUB_RUN_NUMBER: $ENV{GITHUB_RUN_NUMBER}")
else()
  set(WINMENU_VERSION_BUILD 520)
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/VERSION")
  file(READ "${CMAKE_CURRENT_LIST_DIR}/VERSION" WINMENU_LOCAL_VERSION)
  message(STATUS "baulk detect VERSION: ${WINMENU_LOCAL_VERSION}")
  string(REPLACE "." ";" VERSION_LIST ${WINMENU_LOCAL_VERSION})
  list(GET VERSION_LIST 0 WINMENU_VERSION_MAJOR)
  list(GET VERSION_LIST 1 WINMENU_VERSION_MINOR)
  list(GET VERSION_LIST 2 WINMENU_VERSION_PATCH)
  message(STATUS "baulk detect ${WINMENU_VERSION_MAJOR}.${WINMENU_VERSION_MINOR}.${WINMENU_VERSION_PATCH}")
else()
  set(WINMENU_VERSION_MAJOR 1)
  set(WINMENU_VERSION_MINOR 0)
  set(WINMENU_VERSION_PATCH 0)
endif()

set(PACKAGE_VERSION "${WINMENU_VERSION_MAJOR}.${WINMENU_VERSION_MINOR}.${WINMENU_VERSION_PATCH}")

string(TOLOWER "${CMAKE_C_COMPILER_ARCHITECTURE_ID}" WINMENU_ARCH_NAME)
set(CPACK_SYSTEM_NAME "win-${WINMENU_ARCH_NAME}")
set(CPACK_PACKAGE_NAME "WinMenu")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "WinMenu")
set(CPACK_PACKAGE_VERSION_MAJOR ${WINMENU_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${WINMENU_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${WINMENU_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})
set(CPACK_PACKAGE_VENDOR "WinMenu")
set(CPACK_PACKAGE_CONTACT "Baulk contributors")

include(CPack)
# Generate version code
configure_file(${CMAKE_SOURCE_DIR}/include/version.h.cmake ${CMAKE_BINARY_DIR}/include/version.h)

include_directories("${CMAKE_BINARY_DIR}/include" "./include")

add_subdirectory(extensions)

