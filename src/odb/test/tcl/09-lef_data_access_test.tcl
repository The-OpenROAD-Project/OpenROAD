set tcl_dir [file dirname [file normalize [info script]]]
set tests_dir [file dirname $tcl_dir]
set data_dir [file join $tests_dir "data"]
source [file join $tcl_dir "test_helpers.tcl"]

# Open database and load LEF
set db [odb::dbDatabase_create]
odb::read_lef $db [file join $data_dir "gscl45nm.lef"]
set tech [$db getTech]
# Basic LEF checks"
check "lef version" {$tech getLefVersion} "5.5"
check "lef version string" {$tech getLefVersionStr} "5.5"

check "manufacturing grid size" {$tech getManufacturingGrid} 5
check "case sensitive" {$tech getNamesCaseSensitive} ON
check "num routing layers" {$tech getRoutingLayerCount} 10 
check "num vias" {$tech getViaCount} 14
check "num layers" {$tech getLayerCount} 22
check "units" {set units [$tech getLefUnits]} 2000

# Via rules checks
set via_rules [$tech getViaGenerateRules]

check "Number of via rules" {llength $via_rules} 10

set via_rule [lindex $via_rules 0]
check "via rule name" {$via_rule getName} "M2_M1"
check "via_rule default" {$via_rule isDefault} 0

check "via_rule layer count" {$via_rule getViaLayerRuleCount} 3

set viaLayerRule [$via_rule getViaLayerRule 0]

set lower_rule [$via_rule getViaLayerRule 0]
set upper_rule [$via_rule getViaLayerRule 1]
set cut_rule   [$via_rule getViaLayerRule 2]

# Check layer names
set lower_layer [$lower_rule getLayer]
set upper_layer [$upper_rule getLayer]
set cut_layer   [$cut_rule getLayer]

check "via M2_M1 lower" {$lower_layer getName} "metal1"
check "via M2_M1 upper" {$upper_layer getName} "metal2"
check "via M2_M1 cut"   {$cut_layer getName} "via1"

# Check via rule details
check "lower has enclosure" {$lower_rule hasEnclosure} 1
check "lower has rect"      {$lower_rule hasRect} 0
check "lower has spacing"   {$lower_rule hasSpacing} 0
check "upper has enclosure" {$upper_rule hasEnclosure} 1
check "upper has rect"      {$upper_rule hasRect} 0
check "upper has spacing"   {$upper_rule hasSpacing} 0
check "cut has enclosure"   {$cut_rule hasEnclosure} 0
check "cut has rect"        {$cut_rule hasRect} 1
check "cut has spacing"     {$cut_rule hasSpacing} 1

check "lower enclosure" {$lower_rule getEnclosure} [lmap x "0 0.035" {expr round($x * $units)}]
check "upper enclosure" {$upper_rule getEnclosure} [lmap x "0 0.035" {expr round($x * $units)}]

set cut_rect [$cut_rule getRect]
check "cut rect"        {list [$cut_rect xMin] [$cut_rect yMin] [$cut_rect xMax] [$cut_rect yMax]}      [lmap x "-0.0325 -0.0325 0.0325 0.0325" {expr round($x * $units)}]
check "cut spacing"     {$cut_rule getSpacing}     [lmap x "0.14 0.14" {expr round($x * $units)}]

set layers [$tech getLayers]
check "returned layers" {llength $layers} 22

set layer [lindex $layers 2]
check "layer name" {$layer getName} "metal1"
check "layer below" {[$layer getLowerLayer] getName} "contact"
check "layer above" {[$layer getUpperLayer] getName} "via1"

check "layer hasAlias"              {$layer hasAlias} 0
check "layer hasArea"               {$layer hasArea} 0
check "layer hasDefaultAntennaRule" {$layer hasDefaultAntennaRule} 0
check "layer hasMaxWidth"           {$layer hasMaxWidth} 0
check "layer hasMinStep"            {$layer hasMinStep} 0
check "layer hasOxide2AntennaRule"  {$layer hasOxide2AntennaRule} 0
check "layer hasProtrusion"         {$layer hasProtrusion} 0
check "layer hasV55SpacingRules"    {$layer hasV55SpacingRules} 0

check "layer type" {$layer getType} "ROUTING"
check "layer direction" {$layer getDirection} "HORIZONTAL"
check "layer pitch" {$layer getPitch} [expr round(0.19 * $units)]
check "layer width" {$layer getWidth} [expr round(0.065 * $units)]
check "layer spacing" {$layer getSpacing} [expr round(0.065 * $units)]
check "layer resistance" {$layer getResistance} 0.38
check "layer capacitance" {$layer getCapacitance} 0.0

exit_summary
