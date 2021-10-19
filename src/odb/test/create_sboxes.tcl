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
set rects {}
for {set i 0} {$i < 10} {incr i} {
    set start [expr {$i * 10}]
    set end [expr {100 + $start}]
    set rect [odb::Rect]
    $rect init $start $start $end $end
    lappend rects $rect
}
set points {}
for {set i 200} {$i <= 1000} {set i [expr {$i + 100}]} {
    set point [odb::Point]
    $point setX $i
    $point setY $i
    lappend points $point
}
odb::createSBoxes $swire $m1 $rects NONE
odb::createSBoxes $swire $v1 $points NONE
set wires [$swire getWires]
foreach box $wires {
    puts "[$box xMin] [$box yMin] [$box xMax] [$box yMax]"
}