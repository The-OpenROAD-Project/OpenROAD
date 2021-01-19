source "helpers.tcl"

set db [ord::get_db]
set lib [odb::read_lef $db "data/gscl45nm.lef"]
if {$lib == "NULL"} {
    puts "FAIL: Failed to read LEF file"
    exit 1
}

puts "pass"
exit 0
