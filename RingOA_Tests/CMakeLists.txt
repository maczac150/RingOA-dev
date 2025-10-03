# =========================
# Build RingOA_Tests
# =========================

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
  wm/owm_test.cpp
  wm/oquantile_test.cpp
  fm_index/ofmi_test.cpp
  test_main.cpp
)

add_executable(test_RingOA ${TEST_SOURCES})

target_include_directories(test_RingOA
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_compile_options(test_RingOA PRIVATE
  $<$<CONFIG:Debug>:-Og -g -Wall -Wextra>
  $<$<CONFIG:RelWithDebInfo>:-O2 -g -fno-omit-frame-pointer -march=native>
  $<$<CONFIG:Release>:-O2 -march=native -flto -funroll-loops>
)

target_link_libraries(test_RingOA
  PRIVATE
    RingOA::RingOA
    oc::tests_cryptoTools
)
