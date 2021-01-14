source "helpers.tcl"

set db [odb::dbDatabase_create]
odb::read_lef $db "data/gscl45nm.lef"
odb::read_def $db "data/design.def"
if {[$db getChip] == "NULL"} {
    puts "FAIL: Read DEF Failed"
    exit 1
}

puts "pass"
exit 0
