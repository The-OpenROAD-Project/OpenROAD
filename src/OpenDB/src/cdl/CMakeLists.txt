add_library(cdl
    cdl.cpp
)

target_include_directories(cdl
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include/opendb
)

set_target_properties(cdl
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)
