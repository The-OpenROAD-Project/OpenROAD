source "helpers.tcl"

set db [ord::get_db]
read_lef "data/gscl45nm.lef"
set lib [$db findLib gscl45nm]
if {$lib == "NULL"} {
    puts "FAIL: Failed to read LEF file"
    exit 1
}

puts "pass"
exit 0
