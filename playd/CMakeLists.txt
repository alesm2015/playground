cmake_minimum_required(VERSION 3.15)
include(CMakePrintHelpers)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

list (APPEND playd_SOURCES
      main.cpp
    )

# set the project name and version
project(playd LANGUAGES C CXX)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14.0)
    message(FATAL_ERROR "GCC version must be at least 14.0!  " ${CMAKE_CXX_COMPILER_VERSION})
endif()

# configure a header file to pass the version number only
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)
configure_file(version.h.in version.h)

set(CMAKE_COLOR_MAKEFILE    ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_BUILD_TYPE        Debug)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(${PROJECT_NAME} ${playd_SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

if (BUILD_DAEMON)
  target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_AS_DAEMON)
endif()

if (BUILD_WITH_PROFILER)
  target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_WITH_PROFILER)
endif()


# Compiler & Linker flags
if (MSVC)
	# Compiling with Microsoft Visual Studio
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
else()
	# Compiling with GCC
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
  if (BUILD_WITH_PROFILER)
    target_compile_definitions(${PROJECT_NAME} PRIVATE CMAKE_CXX_FLAGS=-pg)
    target_compile_definitions(${PROJECT_NAME} PRIVATE CMAKE_EXE_LINKER_FLAGS=-pg)
    target_compile_definitions(${PROJECT_NAME} PRIVATE CMAKE_SHARED_LINKER_FLAGS=-pg)
  endif()
endif()

# Predprocessor definitions
if (MSVC)
	target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WINDOWS)
else()
	target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_UNIX)
endif()


target_include_directories(${PROJECT_NAME} PUBLIC
    include
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    ${CMAKE_CURRENT_BINARY_DIR}
    ${booker_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME}
  booker
)


