add_library(zutil
    misc_functions.cpp
    files.cpp
    name.cpp
    mem.cpp
    parse.cpp
    graph.cpp
    poly_decomp.cpp
)

target_include_directories(zutil
    PUBLIC
        $<INSTALL_INTERFACE:include/opendb>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/opendb>
        ${TCL_INCLUDE_PATH}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/zutil
)

target_compile_features(zutil PRIVATE cxx_auto_type)
target_compile_options(zutil PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)
set_property(TARGET zutil PROPERTY POSITION_INDEPENDENT_CODE ON)
