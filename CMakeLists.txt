cmake_minimum_required(VERSION 3.15)
include(CMakePrintHelpers)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

list (APPEND traderd_SOURCES
      main.cpp
    )

# set the project name and version
project(play LANGUAGES C CXX)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14.0)
    message(FATAL_ERROR "GCC version must be at least 14.0!  " ${CMAKE_CXX_COMPILER_VERSION})
endif()

# configure a header file to pass the version number only
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)
# configure_file(version.h.in version.h)

set(CMAKE_COLOR_MAKEFILE    ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_BUILD_TYPE        Debug)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(src)

add_executable(${PROJECT_NAME} ${traderd_SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)

target_include_directories(${PROJECT_NAME} PUBLIC
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_SOURCE_DIR}"
        ${CMAKE_CURRENT_BINARY_DIR}
        ${main_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME}
  main
)


