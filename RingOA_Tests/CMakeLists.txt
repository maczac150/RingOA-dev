# ############################
# Build RingOA_Tests
# ############################

# Set the project name
project(RingOA_Tests)

# Add the test sources
set(TEST_SOURCES
    utils/utils_test.cpp
    utils/timer_test.cpp
    utils/network_test.cpp
    utils/file_io_test.cpp
    fss/prg_test.cpp
    fss/dpf_test.cpp
    fss/dcf_test.cpp
    sharing/additive_2p_test.cpp
    sharing/binary_2p_test.cpp
    sharing/additive_3p_test.cpp
    sharing/binary_3p_test.cpp
    protocol/equality_test.cpp
    protocol/integer_comparison_test.cpp
    protocol/zt_test.cpp
    protocol/dpf_pir_test.cpp
    protocol/obliv_access_test.cpp
    protocol/ddcf_test.cpp
    wm/wm_test.cpp
    wm/secure_wm_test.cpp
    fm_index/secure_fmi_test.cpp
    test_main.cpp
)

# Add the executable target
add_executable(test_RingOA ${TEST_SOURCES})

# Include directories
target_include_directories(test_RingOA PRIVATE ${CMAKE_SOURCE_DIR})

# Link the libraries
target_link_libraries(test_RingOA
    RingOA
    oc::tests_cryptoTools
)
