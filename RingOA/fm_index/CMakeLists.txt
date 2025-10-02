# =========================
# Build fm_index
# =========================

# Add library
add_library(fm_index STATIC
    sotfmi.cpp
    ofmi.cpp
)

# Include directories
target_include_directories(fm_index
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/RingOA>
)

# Link libraries
target_link_libraries(fm_index
    PUBLIC
    utils
    protocol
    wm
)
