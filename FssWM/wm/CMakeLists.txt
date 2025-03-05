# ############################
# Build wm
# ############################

# Add library
add_library(wm
    obliv_select.cpp
    fsswm.cpp
    wavelet_matrix.cpp
    fmindex.cpp
)

# Include directories
target_include_directories(wm PRIVATE ${CMAKE_SOURCE_DIR})

# Link libraries
target_link_libraries(wm PRIVATE utils)
