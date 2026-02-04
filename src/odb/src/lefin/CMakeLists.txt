# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

add_library(lefin
    lefin.cpp
    reader.cpp
    lefTechLayerSpacingEolParser.cpp
    lefTechLayerWrongDirSpacingParser.cpp
    lefTechLayerMinStepParser.cpp
    lefTechLayerMinWidthParser.cpp
    lefTechLayerCornerSpacingParser.cpp
    lefTechLayerSpacingTablePrlParser.cpp
    lefTechLayerRightWayOnGridOnlyParser.cpp
    lefTechLayerRectOnlyParser.cpp
    lefTechLayerCutClassParser.cpp
    lefTechLayerCutSpacingParser.cpp
    lefTechLayerCutSpacingTableParser.cpp
    lefTechLayerCutEnclosureParser.cpp
    lefTechLayerTypeParser.cpp
    lefTechLayerEolExtensionParser.cpp
    lefTechLayerEolKeepOutRuleParser.cpp
    lefTechLayerAreaRuleParser.cpp
    lefTechLayerPitchRuleParser.cpp
    lefTechLayerForbiddenSpacingRuleParser.cpp
    lefTechLayerTwoWiresForbiddenSpcRuleParser.cpp
    lefTechLayerVoltageSpacing.cpp
    lefMacroClassParser.cpp
    lefMacroEdgeTypeParser.cpp
    CellEdgeSpacingTableParser.cpp
    AntennaGateParser.cpp
    ArraySpacingParser.cpp
    WidthTableParser.cpp
    MinCutParser.cpp
    MetalWidthViaMapParser.cpp
    KeepOutZoneParser.cpp
    MaxSpacingParser.cpp
)

target_include_directories(lefin
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${TCL_INCLUDE_PATH}
)

set_target_properties(lefin
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(lefin
    PUBLIC
        db
        zutil
        lef
        lefzlib
        utl_lib
        Boost::boost
)
