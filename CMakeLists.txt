cmake_minimum_required(VERSION 3.18)

# Set the project name and language
project(RingOA VERSION 1.0.0 LANGUAGES CXX)

# Enable verbose output
set(CMAKE_VERBOSE_MAKEFILE ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-attributes")

# ############################
# Dependencies
# ############################

# Set third-party directory
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/thirdparty)

# Find Boost library
find_package(Boost REQUIRED HINTS ${THIRD_PARTY_DIR}/unix)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost library not found. Please install it or set the path correctly.")
endif()

# Find cryptoTools library
find_package(cryptoTools REQUIRED HINTS ${THIRD_PARTY_DIR}/unix)

if(NOT cryptoTools_FOUND)
    message(FATAL_ERROR "cryptoTools library not found. Please install it or set the path correctly.")
endif()

# ############################
# Build Settings
# ############################

# Check build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT CMAKE_BUILD_TYPE MATCHES "Debug|Release|Profile")
    message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}. Supported types are Debug and Release.")
endif()

# Set build flags
if(CMAKE_BUILD_TYPE MATCHES Release)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -march=native -flto -funroll-loops")
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og -g -Wall -Wextra")
    add_compile_definitions(USE_FIXED_RANDOM_SEED)
elseif(CMAKE_BUILD_TYPE MATCHES Profile)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -O2 -g -flto -fno-omit-frame-pointer")
endif()

# ############################
# Logging Level Settings
# ############################

# Set default log levels based on build type
if(NOT DEFINED LOG_LEVEL) # LOG_LEVELがユーザーによって指定されていない場合のみ設定
    if(CMAKE_BUILD_TYPE MATCHES Release)
        set(LOG_LEVEL 4) # INFO
        set(USE_FIXED_RANDOM_SEED 0)
    elseif(CMAKE_BUILD_TYPE MATCHES Debug)
        set(LOG_LEVEL 5) # DEBUG
        add_compile_definitions(USE_FIXED_RANDOM_SEED)
    elseif(CMAKE_BUILD_TYPE MATCHES Profile)
        set(LOG_LEVEL 0) # NONE
    endif()
endif()

# Pass the LOG_LEVEL definition to the compiler
add_compile_definitions(LOG_LEVEL=${LOG_LEVEL})

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
message(STATUS "  cryptoTools Directory     : ${cryptoTools_DIR}")
message(STATUS "  Boost Version             : ${Boost_VERSION}")
message(STATUS "  Boost Include Directories : ${Boost_INCLUDE_DIRS}")
message(STATUS "  Boost Libraries           : ${Boost_LIBRARIES}")
message(STATUS "  Boost Library Dirs        : ${Boost_LIBRARY_DIRS}")
message(STATUS "[Settings]")
message(STATUS "  LOG_LEVEL                 : ${LOG_LEVEL}")
message(STATUS "--------------------------------------------")

# ############################
# Build RingOA
# ############################

# Add the source files
add_subdirectory(RingOA)
add_subdirectory(RingOA_Tests)
add_subdirectory(RingOA_Bench)
