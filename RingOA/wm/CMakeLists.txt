# ############################
# Build wm
# ############################

# Add library
add_library(wm
    plain_wm.cpp
    sotwm.cpp
    owm.cpp
    oquantile.cpp
)

# Include directories
target_include_directories(wm PRIVATE ${CMAKE_SOURCE_DIR})

# Link libraries
target_link_libraries(wm PRIVATE
    utils
    protocol
    sdsl
    divsufsort
    divsufsort64
)
