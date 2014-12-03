
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

add_definitions(-DBOOST_LOG_DYN_LINK) # fix boost log link errors

find_package(Boost REQUIRED program_options thread log)
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
    )
set(VPD_SRCS
    main.cpp
    )
add_executable(vpd ${VPD_SRCS})
target_link_libraries(vpd 
    ${Boost_LIBRARIES} ${Boost_LIBRARIES}
    pthread)
