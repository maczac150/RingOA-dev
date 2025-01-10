# ############################
# Build FssWM_Bench
# ############################

# Set the project name
project(FssWM_Bench)

# Add the bench sources
set(BENCH_SOURCES
    dpf_bench.cpp
    bench_main.cpp
)

# Add the executable target
add_executable(bench_FssWM ${BENCH_SOURCES})

# Include directories
target_include_directories(bench_FssWM PRIVATE ${CMAKE_SOURCE_DIR})

# Link the libraries
target_link_libraries(bench_FssWM
    FssWM
    oc::tests_cryptoTools
)
