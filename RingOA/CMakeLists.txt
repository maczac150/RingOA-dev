# =========================
# Build RingOA
# =========================

# Create the unified static library
add_library(RingOA STATIC)
add_library(RingOA::RingOA ALIAS RingOA)

# Ensure consumers get C++20
target_compile_features(RingOA PUBLIC cxx_std_20)

# All sources go directly into RingOA
target_sources(RingOA PRIVATE
  # utils
  utils/logger.cpp
  utils/timer.cpp
  utils/network.cpp

  # sharing
  sharing/additive_2p.cpp
  sharing/additive_3p.cpp
  sharing/binary_2p.cpp
  sharing/binary_3p.cpp

  # fss
  fss/dcf_eval.cpp
  fss/dcf_gen.cpp
  fss/dcf_key.cpp
  fss/dpf_eval.cpp
  fss/dpf_gen.cpp
  fss/dpf_key.cpp
  fss/fss.cpp
  fss/prg.cpp

  # protocol
  protocol/ddcf.cpp
  protocol/dpf_pir.cpp
  protocol/equality.cpp
  protocol/integer_comparison.cpp
  protocol/obliv_select.cpp
  protocol/ringoa.cpp
  protocol/shared_ot.cpp
  protocol/zero_test.cpp

  # wm
  wm/oquantile.cpp
  wm/owm.cpp
  wm/plain_wm.cpp
  wm/sotwm.cpp

  # fm_index
  fm_index/ofmi.cpp
  fm_index/sotfmi.cpp
)

# Include headers from the RingOA/ tree
target_include_directories(RingOA
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Link external deps only once here
target_link_libraries(RingOA
  PUBLIC
    oc::cryptoTools
    sdsl
    divsufsort
    divsufsort64
)

set_target_properties(RingOA PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  VERSION   ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  EXPORT_NAME RingOA
)

# ---- Install public headers ----
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/RingOA
  FILES_MATCHING
    PATTERN "*.h"  PATTERN "*.hpp" PATTERN "*.hh" PATTERN "*.ipp"
    PATTERN ".git" EXCLUDE
    PATTERN "CMakeFiles" EXCLUDE
)

# ---- Install the library ----
install(TARGETS RingOA
  EXPORT ${RINGOA_EXPORT_SET}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/RingOA
)
