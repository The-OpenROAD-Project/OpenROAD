add_library(lefout
    lefout.cpp
)

target_include_directories(lefout
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include/opendb
        ${TCL_INCLUDE_PATH}
)

target_link_libraries(lefout
    utl
)

set_target_properties(lefout
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)
