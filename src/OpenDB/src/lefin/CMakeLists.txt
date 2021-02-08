find_package(Boost)

add_library(lefin
    lefin.cpp
    reader.cpp
    lefTechLayerSpacingEolParser.cpp
    lefTechLayerMinStepParser.cpp
    lefTechLayerCornerSpacingParser.cpp
    lefTechLayerSpacingTablePrlParser.cpp
    lefTechLayerRightWayOnGridOnlyParser.cpp
    lefTechLayerRectOnlyParser.cpp
    lefTechLayerCutClassParser.cpp
    lefTechLayerCutSpacingParser.cpp
    lefTechLayerCutSpacingTableParser.cpp
    lefTechLayerTypeParser.cpp
)

target_include_directories(lefin
    PUBLIC
        $<INSTALL_INTERFACE:include/opendb>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/opendb>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/lefin
        ${Boost_INCLUDE_DIRS}
)

target_compile_features(lefin PRIVATE cxx_auto_type)
target_compile_options(lefin PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)
set_property(TARGET lefin PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(lefin
    PUBLIC
        zutil
        lef
        utility
)
