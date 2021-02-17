add_library(tm
    tm.cpp
    ZFactory.cpp
    ZNamespace.cpp
)

target_include_directories(tm
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include/opendb
)

target_link_libraries(tm
    PUBLIC
        tcl
)

set_target_properties(tm
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)
