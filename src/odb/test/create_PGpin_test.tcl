source "helpers.tcl"

set db [ord::get_db]
read_lef "data/gscl45nm.lef"
read_def "data/parser_test.def"

set chip [$db getChip]
set block [$chip getBlock]
set tech [$db getTech]

odb::createPGpins $block $tech VDD 3 0

set b0 [$block findBTerm pg_VDD_0]
set bpin0 [$b0 getBPins]
set bpin0 [lindex $bpin0 0]
set bpin0 [$bpin0 getBBox]

set b1 [$block findBTerm pg_VDD_1]
set bpin1 [$b1 getBPins]
set bpin1 [lindex $bpin1 0]
set bpin1 [$bpin1 getBBox]

set b2 [$block findBTerm pg_VDD_2]
set bpin2 [$b2 getBPins]
set bpin2 [lindex $bpin2 0]
set bpin2 [$bpin2 getBBox]

puts "[$bpin0 xMin] [$bpin0 yMin] [$bpin0 xMax] [$bpin0 yMax]"
puts "[$bpin1 xMin] [$bpin1 yMin] [$bpin1 xMax] [$bpin1 yMax]"
puts "[$bpin2 xMin] [$bpin2 yMin] [$bpin2 xMax] [$bpin2 yMax]"

puts "pass"
exit 0