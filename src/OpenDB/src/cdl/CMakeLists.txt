add_library(cdl
    cdl.cpp
)

target_include_directories(cdl
    PUBLIC
        $<INSTALL_INTERFACE:include/opendb>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/opendb>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/cdl
)

target_link_libraries(defin
    PUBLIC
        zutil
        def
)
target_compile_features(cdl PRIVATE cxx_auto_type)
target_compile_options(cdl PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)
set_property(TARGET cdl PROPERTY POSITION_INDEPENDENT_CODE ON)

