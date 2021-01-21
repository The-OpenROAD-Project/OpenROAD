source "helpers.tcl"

set db [ord::get_db]
read_lef "data/gscl45nm.lef"
read_def "data/design.def"
if {[$db getChip] == "NULL"} {
    puts "FAIL: Read DEF Failed"
    exit 1
}

puts "pass"
exit 0
