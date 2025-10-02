# =========================
# Build RingOA_Bench
# =========================

project(RingOA_Bench)

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

target_link_libraries(bench_RingOA
  PRIVATE
    RingOA::RingOA
    oc::tests_cryptoTools
)
