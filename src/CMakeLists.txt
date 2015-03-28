
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib)

#add_definitions(-DBOOST_LOG_DYN_LINK) # fix boost log link errors

find_package(Boost REQUIRED program_options log filesystem system thread regex)
message("Boost_INCLUDE_DIR = " ${Boost_INCLUDE_DIR})
INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIR}
)

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
    )
set(VPD_SRCS
    main.cpp
    runtime_config.cpp
    client_engine.cpp
    play_engine.cpp
    playlist.cpp
    player.cpp
    )
add_executable(vpd ${VPD_SRCS})
message("Boost_LIBRARIES = " ${Boost_LIBRARIES})
target_link_libraries(vpd
    ${Boost_LIBRARIES} ${Boost_LIBRARIES}
    pthread)
install(TARGETS vpd DESTINATION bin)
