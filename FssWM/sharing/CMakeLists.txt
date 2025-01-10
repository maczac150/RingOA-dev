# ############################
# Build sharing
# ############################

# Add library
add_library(sharing STATIC
    beaver_triples.cpp
    additive_2p.cpp
)

# Include directories
target_include_directories(sharing PRIVATE ${CMAKE_SOURCE_DIR})

# Link libraries
target_link_libraries(sharing PRIVATE oc::cryptoTools)
