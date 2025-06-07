# ############################
# Build FssWM_Tests
# ############################

# Set the project name
project(FssWM_Tests)

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
    wm/wm_test.cpp
    wm/obliv_select_test.cpp
    wm/fsswm_test.cpp
    wm/ddcf_test.cpp
    fm_index/zt_test.cpp
    fm_index/fssfmi_test.cpp
    test_main.cpp
)

# Add the executable target
add_executable(test_FssWM ${TEST_SOURCES})

# Include directories
target_include_directories(test_FssWM PRIVATE ${CMAKE_SOURCE_DIR})

# Link the libraries
target_link_libraries(test_FssWM
    FssWM
    oc::tests_cryptoTools
)
