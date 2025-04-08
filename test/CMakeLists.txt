cmake_minimum_required(VERSION 3.8)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

project(play_test)
enable_testing()

#External librarires
set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.73.0 COMPONENTS coroutine unit_test_framework system REQUIRED) 

add_library(standalone_asio_test INTERFACE)

add_executable(
    test_suite
    booking_test.cpp
    parser_test.cpp
)

target_include_directories(test_suite PUBLIC
    test_suite
    include
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${booker_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)

# indicates the shared library variant
target_compile_definitions(test_suite PRIVATE "BOOST_TEST_DYN_LINK=1")

# indicates the link paths
target_link_libraries(test_suite booker ${Boost_LIBRARIES})

# declares a test with our executable
add_test(NAME play_test COMMAND test_suite)
