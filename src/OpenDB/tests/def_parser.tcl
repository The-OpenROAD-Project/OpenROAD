source "helpers.tcl"

set db [ord::get_db]
read_lef "data/gscl45nm.lef"
read_def "data/parser_test.def"
set chip [$db getChip]
if {$chip == "NULL"} {
    puts "FAIL: Read DEF Failed"
    exit 1
}

set block [$chip getBlock]
set out_def "results/parser_test_out.def"
write_def $out_def

set isDiff [diff_files $out_def "parser_test.defok"]
if {$isDiff != 0} {
    exit 1
}

puts "pass"
exit 0
