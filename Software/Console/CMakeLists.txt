# TDB license

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_TOOLCHAIN_FILE "config/apollo-toolchain.cmake")

project(battery-free-console LANGUAGES C CXX ASM)

# Prefix path to install executables
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR})

add_subdirectory(apps)

