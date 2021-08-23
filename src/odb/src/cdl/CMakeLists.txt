add_library(cdl
    cdl.cpp
)

target_include_directories(cdl
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${TCL_INCLUDE_PATH}
)

set_target_properties(cdl
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)
