# ############################
# Build utils
# ############################

# Add library
add_library(utils STATIC
    logger.cpp
    timer.cpp
    file_io.cpp
    network.cpp
)

# Include directories
target_include_directories(utils PUBLIC ${CMAKE_SOURCE_DIR})

# Link libraries
target_link_libraries(utils PUBLIC -lcrypto oc::cryptoTools)
