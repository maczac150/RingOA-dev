# =========================
# Build RingOA_Bench
# =========================

set(BENCH_SOURCES
    dpf_bench.cpp
    dpf_pir_bench.cpp
    obliv_select_bench.cpp
    obliv_access_bench.cpp
    ofmi_bench.cpp
    oquantile_bench.cpp
    bench_main.cpp
)

add_executable(bench_RingOA ${BENCH_SOURCES})

# Include directories
target_include_directories(bench_RingOA
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_compile_options(bench_RingOA PRIVATE
  $<$<CONFIG:Debug>:-Og -g -Wall -Wextra>
  $<$<CONFIG:RelWithDebInfo>:-O2 -g -fno-omit-frame-pointer -march=native>
  $<$<CONFIG:Release>:-O2 -march=native -flto -funroll-loops>
)


target_link_libraries(bench_RingOA
  PRIVATE
    RingOA::RingOA
    oc::tests_cryptoTools
)
