cmake_minimum_required(VERSION 2.8.9)
project(vpd)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9")
    message(FATAL_ERROR "Insufficient gcc version")
  endif()
endif()

set(VPD_MAJOR_VERSION 0)
set(VPD_MINOR_VERSION 1)
ADD_DEFINITIONS(
    -std=c++14
    )
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
set(CMAKE_BUILD_TYPE Debug)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(doc)

