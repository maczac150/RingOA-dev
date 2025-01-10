# ############################
# Build FssWM
# ############################

# Set the project name
project(FssWM)

# Add subdirectories
add_subdirectory(utils)
add_subdirectory(sharing)
add_subdirectory(fss)

# Add the library target
add_library(FssWM INTERFACE)

# Link the libraries
target_link_libraries(FssWM INTERFACE
    utils
    sharing
    fss
)

# Add the include directories
target_include_directories(FssWM INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/sharing
    ${CMAKE_CURRENT_SOURCE_DIR}/fss
)
