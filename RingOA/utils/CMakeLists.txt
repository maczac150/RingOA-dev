# =========================
# Build utils
# =========================

# Add library
add_library(utils STATIC
  logger.cpp
  timer.cpp
  network.cpp
)

set_target_properties(utils PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(utils 
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/RingOA>
)

target_link_libraries(utils
  PUBLIC
    oc::cryptoTools
)
