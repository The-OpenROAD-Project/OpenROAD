add_library(tm STATIC
    ZFactory.cpp
)

target_include_directories(tm
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_include_directories(tm
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${TCL_INCLUDE_PATH}
)

set_target_properties(tm
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)
