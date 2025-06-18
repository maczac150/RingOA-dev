# ############################
# Build protocol
# ############################

# Add library
add_library(protocol
    ddcf.cpp
    obliv_select.cpp
    mixed_obliv_select.cpp
)

# Include directories
target_include_directories(protocol PRIVATE ${CMAKE_SOURCE_DIR})

# Link libraries
target_link_libraries(protocol PRIVATE
    utils
    sdsl
    divsufsort
    divsufsort64
)
