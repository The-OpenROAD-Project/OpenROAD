source "helpers.tcl"


set db [odb::dbDatabase_create]
set lib [odb::read_lef $db  "data/gscl45nm.lef"]
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
