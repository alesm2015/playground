cmake_minimum_required(VERSION 3.15)
include(CMakePrintHelpers)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

list (APPEND main_SOURCES
      server.cpp
      session.cpp
      booking.cpp
    )

project(main LANGUAGES C CXX)
#External librarires
set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.73.0 COMPONENTS coroutine REQUIRED) 

# Boost.Coroutine2 (the successor of Boost.Coroutine)
# (1) doesn't even exist in old Boost versions and
# (2) isn't supported by ASIO, yet.
add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)

include(FetchContent)
FetchContent_Declare(
  cli
  GIT_REPOSITORY https://github.com/daniele77/cli.git
  GIT_TAG v2.2.0
)
FetchContent_MakeAvailable(cli)


set(CMAKE_COLOR_MAKEFILE    ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_BUILD_TYPE        Debug)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(${PROJECT_NAME} STATIC ${main_SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)

#target_link_directories(${PROJECT_NAME} PUBLIC
#                        ${LOCAL_3RDPTY_LIBRARY_DIRS})

target_include_directories(${PROJECT_NAME} PUBLIC
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    cli::cli
)



