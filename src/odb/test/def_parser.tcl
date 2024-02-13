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
set out_def [make_result_file "def_parser.def"]
write_def $out_def

diff_files $out_def "def_parser.defok"

puts "pass"
exit 0
