add_library(upf
    upf.cpp
)

target_include_directories(upf
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${TCL_INCLUDE_PATH}
)

set_target_properties(upf
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(upf
    PUBLIC
        utl
)
