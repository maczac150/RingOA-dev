# ############################
# Build fss
# ############################

# Add library
add_library(fss
    fss.cpp
    prg.cpp
    dpf_key.cpp
    dpf_gen.cpp
    dpf_eval.cpp
    libPSI_dpf_gen.cpp
    libPSI_dpf_eval.cpp
)

# Include directories
target_include_directories(fss PRIVATE ${CMAKE_SOURCE_DIR})

# Link libraries
target_link_libraries(fss PRIVATE emp-tool oc::cryptoTools utils)
