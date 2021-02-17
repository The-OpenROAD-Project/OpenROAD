add_library(zlib
    ZException.cpp
)

target_include_directories(zlib
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/opendb
    ${TCL_INCLUDE_PATH}
)

set_target_properties(zlib
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)
