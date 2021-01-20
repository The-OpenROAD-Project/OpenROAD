source "helpers.tcl"


set db [ord::get_db]
read_lef "data/gscl45nm.lef"
set lib [$db findLib gscl45nm]
if {$lib == "NULL"} {
    puts "FAIL: Failed to read LEF file"
    exit 1
}
set tech [$lib getTech]
if {$tech == "NULL"} {
    puts "FAIL: Failed to get tech"
}
set via_rules [$tech getViaRules]
puts "pass"
exit 0
