source "helpers.tcl"

set db [ord::get_db]
read_lef "data/gscl45nm.lef"
read_def "data/parser_test.def"

set chip [$db getChip]
set block [$chip getBlock]
set tech [$db getTech]
set m1 [$tech findLayer metal1]
set n1 [$block findNet VDD]
set v1 [$block findVia VIA1]
set swire [odb::dbSWire_create $n1 NONE ]
set rect [odb::Rect]
$rect init 0 0 100 100
odb::createSBoxes $swire $m1 {$rect} NONE
set point [odb::Point]
$point setX 200
$point setY 200
odb::createSBoxes $swire $v1 {$point} NONE
set wires [$swire getWires]
foreach box $wires {
    puts "[$box xMin] [$box yMin] [$box xMax] [$box yMax]"
}