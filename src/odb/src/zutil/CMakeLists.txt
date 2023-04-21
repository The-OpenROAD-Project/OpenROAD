add_library(zutil STATIC
    parse.cpp
    poly_decomp.cpp
    util.cpp
)

target_link_libraries(zutil
  PUBLIC
        db
        utl_lib
)

target_include_directories(zutil
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/odb
    ${TCL_INCLUDE_PATH}
)

set_target_properties(zutil
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)
