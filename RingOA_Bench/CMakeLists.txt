# ############################
# Build RingOA_Bench
# ############################

# Set the project name
project(RingOA_Bench)

# Add the bench sources
set(BENCH_SOURCES
    dpf_bench.cpp
    dpf_pir_bench.cpp
    obliv_select_bench.cpp
    obliv_access_bench.cpp
    ofmi_bench.cpp
    bench_main.cpp
)

# Add the executable target
add_executable(bench_RingOA ${BENCH_SOURCES})

# Include directories
target_include_directories(bench_RingOA PRIVATE ${CMAKE_SOURCE_DIR})

# Link the libraries
target_link_libraries(bench_RingOA
    RingOA
    oc::tests_cryptoTools
)
