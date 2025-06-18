# ############################
# Build wm
# ############################

# Add library
add_library(wm
    plain_wm.cpp
    fsswm.cpp
)

# Include directories
target_include_directories(wm PRIVATE ${CMAKE_SOURCE_DIR})

# Link libraries
target_link_libraries(wm PRIVATE
    utils
    sdsl
    divsufsort
    divsufsort64
)
