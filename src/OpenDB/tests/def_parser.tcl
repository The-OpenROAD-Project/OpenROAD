source "helpers.tcl"

set db [ord::get_db]
odb::read_lef $db "data/gscl45nm.lef"
odb::read_def $db "data/parser_test.def"
set chip [$db getChip]
if {$chip == "NULL"} {
    puts "FAIL: Read DEF Failed"
    exit 1
}

set block [$chip getBlock]
set out_def "results/parser_test_out.def"
set def_write_result [odb::write_def $block $out_def]
if {$def_write_result != 1} {
    exit 1
}

set isDiff [diff_files $out_def "parser_test.defok"]
if {$isDiff != 0} {
    exit 1
}

puts "pass"
exit 0
