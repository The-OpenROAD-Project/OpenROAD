source "helpers.tcl"


set db [odb::dbDatabase_create]
set lib [odb::read_lef $db "data/gscl45nm.lef"]
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
