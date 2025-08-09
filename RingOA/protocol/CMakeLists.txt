# ############################
# Build protocol
# ############################

# Add library
add_library(protocol
    ddcf.cpp
    equality.cpp
    integer_comparison.cpp
    zero_test.cpp
    dpf_pir.cpp
    obliv_select.cpp
    ringoa.cpp
    shared_ot.cpp
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
