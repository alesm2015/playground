cmake_minimum_required(VERSION 3.15)
include(CMakePrintHelpers)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Doxygen)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

# Cmake configuration
option(BUILD_UNIT_TESTS "Build unit tests" ON)
option(BUILD_DOCUMENTATION "Create and install the HTML based API        
documentation (requires Doxygen)" ${DOXYGEN_FOUND})

# set the project name and version
project(demo)


# Static library of entire project
add_subdirectory(booker)

# Build the application
add_subdirectory(playd)

# Run unit tests
if (BUILD_UNIT_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

# Build documentation
if(BUILD_DOCUMENTATION)
    if(NOT DOXYGEN_FOUND)
         message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif()

    set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/doc/doxy/doxyfile.doxygen)

    #cmake_print_variables(doxyfile_in)

    add_custom_target(
      doc ALL
      COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile_in}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM
    )
endif()

