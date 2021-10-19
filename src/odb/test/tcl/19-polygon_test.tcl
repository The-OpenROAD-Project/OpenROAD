set tcl_dir [file dirname [file normalize [info script]]]
source [file join $tcl_dir "test_helpers.tcl"]

set ps1 [odb::newSetFromRect 0 0 20 10]
set ps2 [odb::newSetFromRect 0 0 10 20]
set or [odb::orSet $ps1 $ps2]

set test []
foreach poly [odb::getPolygons $or] {
    set pts []
    foreach pt [odb::getPoints $poly] {
        lappend pts [$pt getX] [$pt getY]
    }
    lappend test $pts
}

check "polygon points" {list $test} {{{0 0 20 0 20 10 10 10 10 20 0 20}}}

set test []
foreach rect [odb::getRectangles $or] {
    lappend test [list [$rect xMin] [$rect yMin] [$rect xMax] [$rect yMax]]
}

check "rectangles" {list $test} {{{0 0 20 10} {0 10 10 20}}}

set bloatXY [odb::bloatSet $ps1 5 10]
set test []
foreach rect [odb::getRectangles $bloatXY] {
    lappend test [list [$rect xMin] [$rect yMin] [$rect xMax] [$rect yMax]]
}

check "bloatXY" {list $test} {{{-5 -10 25 20}}}
