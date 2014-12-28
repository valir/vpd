find_package(Boost REQUIRED log filesystem system unit_test_framework thread)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ ${CMAKE_CURRENT_BINARY_DIR}/../)

add_executable(test_client_connection client_connection.cpp)
target_link_libraries(test_client_connection
    ${Boost_LIBRARIES} ${Boost_LIBRARIES} pthread)
add_test(test_client_connection test_client_connection)
