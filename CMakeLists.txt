cmake_minimum_required(VERSION 3.0)
project(vpd)

set(VPD_MAJOR_VERSION 0)
set(VPD_MINOR_VERSION 1)
ADD_DEFINITIONS(
    -std=c++11 # remember this must compile on RaspberryPi so refrain from c++14 before until it'll get supported there
    )
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

add_subdirectory(src)
