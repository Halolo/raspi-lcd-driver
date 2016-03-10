cmake_minimum_required(VERSION 2.8)
project(raspi-lcd-control)

set(CMAKE_SYSTEM_PREFIX_PATH "/usr/local")

add_subdirectory(liblcd)
add_subdirectory(test)