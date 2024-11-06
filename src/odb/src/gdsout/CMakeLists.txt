add_library(gdsout
    gdsout.cpp
)

target_include_directories(gdsout
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${TCL_INCLUDE_PATH}
)
target_link_libraries(gdsout
    db
    utl_lib
)

set_target_properties(gdsout
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)

