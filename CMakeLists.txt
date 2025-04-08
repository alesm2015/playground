cmake_minimum_required(VERSION 3.15)
include(CMakePrintHelpers)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()


# set the project name and version
project(demo)


# Static library of entire code
add_subdirectory(booker)

# Build the application
add_subdirectory(playd)

# Run unit tests
enable_testing()
add_subdirectory(test)


