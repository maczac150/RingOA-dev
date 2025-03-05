# ############################
# Build sharing
# ############################

# Add library
add_library(sharing STATIC
    beaver_triples.cpp
    additive_2p.cpp
    binary_2p.cpp
    share3_types.cpp
    additive_3p.cpp
    binary_3p.cpp
)

# Include directories
target_include_directories(sharing PRIVATE ${CMAKE_SOURCE_DIR})

# Link libraries
target_link_libraries(sharing PRIVATE oc::cryptoTools)
