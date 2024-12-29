#######################################################
#                     Build FssWM                     #
#######################################################

# Check emp-tool installation
if(NOT EMP-TOOL_INCLUDE_DIRS)
    message(FATAL_ERROR "emp-tool include directories not set. Check emp-tool installation.")
endif()

# Add the source files
file(GLOB_RECURSE LIB_SOURCES fss/*.cpp utils/*.cpp)

# Add the fss library
add_library(fss ${LIB_SOURCES})

# Set compile options for fss
target_compile_features(fss PUBLIC cxx_std_20)
target_compile_options(fss PUBLIC -Wall -maes -msse4.1 -Wno-ignored-attributes)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(fss PUBLIC LOG_LEVEL_DEBUG RANDOM_SEED_FIXED)
else()
    target_compile_definitions(fss PUBLIC LOGGING_ENABLED)
endif()

# Include directories for fss
target_include_directories(fss PUBLIC ${EMP-TOOL_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

# Link libraries for fss
target_link_libraries(fss PUBLIC emp-tool oc::cryptoTools)
