# =========================
# Build sharing
# =========================

# Add library
add_library(sharing STATIC
  additive_2p.cpp
  binary_2p.cpp
  additive_3p.cpp
  binary_3p.cpp
)

set_target_properties(sharing PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(sharing
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/RingOA>
)

target_link_libraries(sharing
  PUBLIC
    utils
    oc::cryptoTools
)
