source "helpers.tcl"


set db [ord::get_db]
read_lef "data/gscl45nm.lef"
read_def "data/design.def"
set chip [$db getChip]
set block [$chip getBlock]
set nets [$block getNets]
foreach net $nets {
    puts "Net: [$net getName]"
}

if {[llength $nets] != 24} {
    puts "FAIL: There should be 24 nets"
    exit 1
}

puts "pass"
exit
