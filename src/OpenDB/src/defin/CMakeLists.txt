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
        ${PROJECT_SOURCE_DIR}/include/opendb
)

target_link_libraries(defin
    PUBLIC
        zutil
        def
        utility
)

set_target_properties(defin
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)

