cmake_minimum_required(VERSION 3.18)

# Set the project name and language
project(FssWM VERSION 1.0.0 LANGUAGES CXX)

# Enable verbose output
set(CMAKE_VERBOSE_MAKEFILE ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

# Find dependencies
# Set third-party directory
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/thirdparty)

# Find Boost library
find_package(Boost REQUIRED HINTS ${THIRD_PARTY_DIR}/unix)
if (NOT Boost_FOUND)
message(FATAL_ERROR "Boost library not found. Please install it or set the path correctly.")
endif()

# Find cryptoTools library
find_package(cryptoTools REQUIRED HINTS ${THIRD_PARTY_DIR}/unix)
if (NOT cryptoTools_FOUND)
message(FATAL_ERROR "cryptoTools library not found. Please install it or set the path correctly.")
endif()

# Find emp-tool library
find_package(emp-tool REQUIRED)
if (NOT emp-tool_FOUND)
    message(FATAL_ERROR "emp-tool library not found. Ensure it is installed on the system.")
endif()

# Set profile flags
set(CMAKE_C_FLAGS_PROFILE "-pg -O2 -g")
set(CMAKE_CXX_FLAGS_PROFILE "-pg -O2 -g")
set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-pg -O2 -g")

# Check build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if (NOT CMAKE_BUILD_TYPE MATCHES "Debug|Release|Profile")
    message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}. Supported types are Debug and Release.")
endif()

# Display project configuration info
message(STATUS "--------------------------------------------")
message(STATUS " Project Configuration Info:")
message(STATUS "--------------------------------------------")
message(STATUS "[Project Information]")
message(STATUS "  Project Name              : ${PROJECT_NAME}")
message(STATUS "  Version                   : ${PROJECT_VERSION}")
message(STATUS "  CMake Version             : ${CMAKE_VERSION}")
message(STATUS "[Build Settings]")
message(STATUS "  Build Type                : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix            : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "[Directories]")
message(STATUS "  Source Directory          : ${CMAKE_SOURCE_DIR}")
message(STATUS "  Binary Directory          : ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "[Libraries]")
message(STATUS "  emp-tool Library          : ${EMP-TOOL_LIBRARIES}")
message(STATUS "  cryptoTools Directory     : ${cryptoTools_DIR}")
message(STATUS "  Boost Version             : ${Boost_VERSION}")
message(STATUS "  Boost Include Directories : ${Boost_INCLUDE_DIRS}")
message(STATUS "  Boost Libraries           : ${Boost_LIBRARIES}")
message(STATUS "  Boost Library Dirs        : ${Boost_LIBRARY_DIRS}")
message(STATUS "--------------------------------------------")

#######################################################
#                     Build FssWM                     #
#######################################################

# Add the source files
add_subdirectory(FssWM)
add_subdirectory(FssWM_Tests)
