# ############################
# Build fm_index
# ############################

# Add library
add_library(fm_index
    sotfmi.cpp
    fssfmi.cpp
)

# Include directories
target_include_directories(fm_index PRIVATE ${CMAKE_SOURCE_DIR})

# Link libraries
target_link_libraries(fm_index PRIVATE
    utils
    protocol
    wm
)
