# =========================
# Build protocol
# =========================

# Add library
add_library(protocol STATIC
    ddcf.cpp
    equality.cpp
    integer_comparison.cpp
    zero_test.cpp
    dpf_pir.cpp
    obliv_select.cpp
    ringoa.cpp
    shared_ot.cpp
)

set_target_properties(protocol PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(protocol
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/RingOA>
)

target_link_libraries(protocol
  PUBLIC
    utils
    fss
    sharing
)
