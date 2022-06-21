source "helpers.tcl"

set db [ord::get_db]
read_db "data/design.odb"
if {[$db getChip] == "NULL"} {
    puts "FAIL: Read ODB Failed"
    exit 1
}

puts "pass"
exit 0
