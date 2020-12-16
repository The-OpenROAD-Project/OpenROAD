add_library(lefout
    lefout.cpp
)

target_include_directories(lefout
    PUBLIC
        $<INSTALL_INTERFACE:include/opendb>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/opendb>
        ${TCL_INCLUDE_PATH}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/lefout
)

target_compile_features(lefout PRIVATE cxx_auto_type)
target_compile_options(lefout PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)
set_property(TARGET lefout PROPERTY POSITION_INDEPENDENT_CODE ON)
