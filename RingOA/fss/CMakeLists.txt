# =========================
# Build fss
# =========================

# Add library
add_library(fss STATIC
    fss.cpp
    prg.cpp
    dpf_key.cpp
    dpf_gen.cpp
    dpf_eval.cpp
    dcf_key.cpp
    dcf_gen.cpp
    dcf_eval.cpp
)

set_target_properties(fss PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(fss
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/RingOA>
)

target_link_libraries(fss
  PUBLIC
    oc::cryptoTools
    utils
)
