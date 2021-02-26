set tcl_dir [file dirname [file normalize [info script]]]
set tests_dir [file dirname $tcl_dir]
set data_dir [file join $tests_dir "data"]
source [file join $tcl_dir "test_helpers.tcl"]

# Open database, load lef and design

set db [odb::dbDatabase_create]
odb::read_lef $db [file join $data_dir "Nangate45" "NangateOpenCellLibrary.mod.lef"]
odb::read_def $db [file join $data_dir "gcd" "gcd_pdn.def"]
set chip [$db getChip]
set block [$chip getBlock]

check "block name" {$block getName} "gcd"

check "def units" {set units [$block getDefUnits]} 2000

check "number of children" {llength [set children [$block getChildren]]} 0
check "number of instances" {llength [set insts [$block getInsts]]} 0
check "number of pins" {llength [set bterms [$block getBTerms]]} 0
check "number of obstructions" {llength [set obstructions [$block getObstructions]]} 0
check "number of blockages"  {llength [set blockages [$block getBlockages]]} 0

check "number of nets" {llength [set nets [$block getNets]]} 2
check "number of vias" {llength [set vias [$block getVias]]} 6
check "number of rows" {llength [set rows [$block getRows]]} 112


set bbox [$block getBBox]
check "bbox" {list [$bbox xMin] [$bbox yMin] [$bbox xMax] [$bbox yMax]} "20140 22230 180120 179370"

check "block gcell grid" {$block getGCellGrid} NULL

set die_area_rect [$block getDieArea]
check "block die area" {list [$die_area_rect xMin] [$die_area_rect yMin] [$die_area_rect xMax] [$die_area_rect yMax]} "0 0 200260 201600"
check "number of regions" {llength [set regions [$block getRegions]]} 0
check "number of nondefault rules" {llength [set non_default_rules [$block getNonDefaultRules]]} 0

# Row checks

set row [lindex $rows 0] 

check "row name" {$row getName} ROW_1
check "row site" {[$row getSite] getName} FreePDK45_38x28_10R_NP_162NW_34O
check "row origin" {$row getOrigin} "20140 22400"
check "row orientation" {$row getOrient} "MX"
check "row direction" {$row getDirection} HORIZONTAL; # Not sure about the expected value here
check "row site count" {$row getSiteCount} 421
check "row site spacing" {$row getSpacing} 380
check "row bbox" {list [[$row getBBox] xMin] [[$row getBBox] yMin] [[$row getBBox] xMax] [[$row getBBox] yMax]} "20140 22400 180120 25200" ; # Guess at the xMax value

# Net checks

set net [lindex $nets 0]

check "net name" {$net getName} VDD
check "net special" {$net isSpecial} 1
check "net weight" {$net getWeight} 1
check "net term count" {$net getTermCount} 0
check "net iterm count" {$net getITermCount} 0
check "net bterm count" {$net getBTermCount} 0

check "net sigType" {$net getSigType} "POWER"
check "net num wires" {llength [set wires [$net getWire]]} 1
check "net num swires" {llength [set swires [$net getSWires]]} 1
check "net num global wire" {llength [set gwires [$net getGlobalWire]]} 1
check "net non default rule" {$net getNonDefaultRule} NULL

# Special wire checks
set swire [lindex $swires 0]

check "swire block" {[$swire getBlock] getName} gcd
check "swire net" {[$swire getNet] getName} VDD
check "swire wire type" {$swire getWireType} ROUTED
check "swire shield" {$swire getShield} NULL

check "swire num wires" {llength [set wires [$swire getWires]]} 219

set wire [lindex $wires 0]
check "wire layer name" {[$wire getTechLayer] getName} metal1
check "wire isVia" {$wire isVia} 0
check "wire width" {$wire getWidth} 340
check "wire length" {$wire getLength} 159980
check "wire shape" {$wire getWireShapeType} FOLLOWPIN
check "wire direction" {$wire getDir} 1 ; # 1 = HORIZONTAL

set wire [lindex $wires 30]
check "wire layer name" {[$wire getTechLayer] getName} metal4
check "wire isVia" {$wire isVia} 0
check "wire width" {$wire getWidth} [expr 179200 - 22400]
check "wire length" {$wire getLength} 960
check "wire shape" {$wire getWireShapeType} STRIPE
check "wire direction" {$wire getDir} 0 ; # 0 = VERTICAL

set wire [lindex $wires 34]
check "wire isVia" {$wire isVia} 1
check "wire techVia" {$wire getTechVia} "NULL" ; # Return value is currently NULL, better to be an empty list
check "wire block via" {[$wire getBlockVia] getName} via2_960x340
check "wire via xy" {$wire getViaXY} "24140 22400"
check "wire num via boxes" {llength [$wire getViaBoxes 0]} 5; # 3 cut shapes + upper and lower metal
check "wire direction" {$wire getDir} 1 ; # A via doesnt really have a direction, so any vaue is okay here I think

set via [lindex $vias 0]

check "via name" {$via getName} via1_960x340
check "via pattern" {$via getPattern} ""
check "via generate rule" {[$via getViaGenerateRule] getName} Via1Array-0
check "via tech via" {$via getTechVia}  "NULL" ; # Return value is currently NULL, better to be an empty list
check "via block via" {$via getBlockVia}  "NULL" ; # Return value is currently NULL, better to be an empty list
check "via num params" {[$via getViaParams] getXCutSize} 140
check "via num shapes" {llength [$via getBoxes]} 5
check "via top layer name" {[$via getTopLayer] getName} metal2
check "via bottom layer name" {[$via getBottomLayer] getName} metal1
check "via rotated" {$via isViaRotated} 0
check "via orientation" {$via getOrient} "R0"

exit_summary
