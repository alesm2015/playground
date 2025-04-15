cmake_minimum_required(VERSION 3.15)
include(CMakePrintHelpers)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

# Files need to be compiled
list (APPEND booker_SOURCES
      server.cpp
      session.cpp
      booking.cpp
      parser.cpp
    )

project(booker LANGUAGES C CXX)
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

include(FetchContent)
FetchContent_Declare(
  libtelnet 
  GIT_REPOSITORY https://github.com/seanmiddleditch/libtelnet.git
  GIT_TAG 0.23
  SOURCE_SUBDIR .
)
FetchContent_MakeAvailable(libtelnet)

add_library(libtelnet STATIC
    ${libtelnet_SOURCE_DIR}/libtelnet.c
    ${libtelnet_SOURCE_DIR}/libtelnet.h
)
target_compile_options(libtelnet PRIVATE -Wall -Wpedantic)


set(CMAKE_COLOR_MAKEFILE    ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_BUILD_TYPE        Debug)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(${PROJECT_NAME} STATIC ${booker_SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

# Compiler & Linker flags
if (MSVC)
	# Compiling with Microsoft Visual Studio
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
else()
	# Compiling with GCC
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Predprocessor definitions
if (MSVC)
	target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WINDOWS)
else()
	target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_UNIX)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    include
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${libtelnet_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
      ${Boost_LIBRARIES}
      cli::cli
      libtelnet
)

set(${PROJECT_NAME}_INCLUDE_DIRS 
    ${Boost_LIBRARIES}
    ${PROJECT_SOURCE_DIR}/include
  CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

