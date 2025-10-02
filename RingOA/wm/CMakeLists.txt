# =========================
# Build wm
# =========================

# Add library
add_library(wm STATIC
    plain_wm.cpp
    sotwm.cpp
    owm.cpp
    oquantile.cpp
)

set_target_properties(wm PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(wm
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/RingOA>
)

target_link_libraries(wm
  PUBLIC
    protocol
  PRIVATE
    utils
    sharing
    sdsl
    divsufsort
    divsufsort64
)
