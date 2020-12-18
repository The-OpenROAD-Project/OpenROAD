add_library(defin
    definNet.cpp 
    definSNet.cpp 
    definComponent.cpp 
    definVia.cpp 
    definPin.cpp 
    definBlockage.cpp 
    definFill.cpp 
    definTracks.cpp 
    definGCell.cpp 
    definRow.cpp 
    definRegion.cpp 
    definNonDefaultRule.cpp 
    definReader.cpp 
    definBase.cpp 
    create_box.cpp 
    defin.cpp 
    definPolygon.cpp 
    definPropDefs.cpp 
    definPinProps.cpp 
)

target_include_directories(defin
    PUBLIC
        $<INSTALL_INTERFACE:include/opendb>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/opendb>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/defin
)

target_link_libraries(defin
    PUBLIC
        zutil
        def
)

target_compile_features(defin PRIVATE cxx_auto_type)
target_compile_options(defin PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall>)

set_property(TARGET defin PROPERTY POSITION_INDEPENDENT_CODE ON)
