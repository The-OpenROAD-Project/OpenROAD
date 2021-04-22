add_library(zutil
    misc_functions.cpp
    files.cpp
    name.cpp
    mem.cpp
    parse.cpp
    poly_decomp.cpp
)

target_include_directories(zutil
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/opendb
    ${TCL_INCLUDE_PATH}
)

set_target_properties(zutil
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)
