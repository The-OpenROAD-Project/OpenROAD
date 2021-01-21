source "helpers.tcl"


set db [ord::get_db]
read_lef "data/gscl45nm.lef"
set lib [$db findLib gscl45nm]
set tech [$lib getTech]
set vias [$tech getVias]

foreach via $vias {
    puts [$via getName]
}

if {[llength $vias] != 14} {
    puts "FAIL: There should be 14 vias"
    exit 1 
}

puts "pass"
exit 0
