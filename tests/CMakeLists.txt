find_package(Boost REQUIRED log filesystem system unit_test_framework thread)

include_directories(${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/lib)

add_executable(test_client_connection client_connection.cpp)
target_link_libraries(test_client_connection
    ${Boost_LIBRARIES} ${Boost_LIBRARIES} pthread)
add_test(test_client_connection test_client_connection)

add_executable(test_playlist playlist.cpp)
target_link_libraries(test_playlist
    ${Boost_LIBRARIES} ${Boost_LIBRARIES} pthread)
add_test(test_playlist test_playlist)

add_executable(test_status status.cpp)
target_link_libraries(test_status
    ${Boost_LIBRARIES} ${Boost_LIBRARIES} pthread)
add_test(test_status test_status)
