cmake_minimum_required(VERSION 3.23)

project(AK10 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "SocketCAN is Linux-only; this project cannot be built on ${CMAKE_SYSTEM_NAME}.")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SocketCAN REQUIRED IMPORTED_TARGET libsocketcan)

add_library(AK10-lib STATIC)
add_library(AK10::AK10-lib ALIAS AK10-lib)

target_sources(AK10-lib
  PRIVATE
    # src/*.cpp
)

target_compile_features(AK10-lib PUBLIC cxx_std_17)

target_include_directories(AK10-lib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(AK10-lib PUBLIC PkgConfig::SocketCAN)

