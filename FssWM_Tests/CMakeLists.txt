#######################################################
#                  Build FssWM_Tests                  #
#######################################################

# Find all source files in the current directory
file(GLOB_RECURSE TESTS_SOURCES *.cpp)

# Remove the main.cpp file from the list of source files
list(REMOVE_ITEM TESTS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

# Add the executables
add_executable(test ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ${TESTS_SOURCES})

# Set compile options for executables
target_compile_features(test PUBLIC cxx_std_20)
target_compile_options(test PRIVATE -Wall -maes -msse4.1 -Wno-ignored-attributes)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(fss PRIVATE LOG_LEVEL_DEBUG RANDOM_SEED_FIXED)
else()
    target_compile_definitions(fss PRIVATE)
endif()


# Include directories for executables
target_include_directories(test PRIVATE ${EMP-TOOL_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Link libraries to the executables
target_link_libraries(test PRIVATE fss -lcrypto ${EMP-TOOL_LIBRARIES} oc::cryptoTools oc::tests_cryptoTools)
