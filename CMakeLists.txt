cmake_minimum_required(VERSION 3.0)
project(vpd)

set(VPD_MAJOR_VERSION 0)
set(VPD_MINOR_VERSION 1)
ADD_DEFINITIONS(
        -std=c++11 # Or -std=c++0x
    )

add_subdirectory(src)
