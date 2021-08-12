set tcl_dir [file dirname [file normalize [info script]]]
set tests_dir [file dirname $tcl_dir]
set data_dir [file join $tests_dir "data"]
set opendb_dir [file dirname $tests_dir]
source [file join $tcl_dir "test_helpers.tcl"]

set db [odb::dbDatabase_create]
odb::read_lef $db [file join $data_dir "gscl45nm.lef"]
odb::read_def $db [file join $data_dir "parser_test.def"]
set chip [$db getChip]
if {$chip == "NULL"} {
    puts "Read DEF Failed"
    exit 1
}

set block [$chip getBlock]
set out_def [file join $opendb_dir "build" "parser_test_out.def"]
set def_write_result [odb::write_def $block $out_def]
if {$def_write_result != 1} {
    exit 1
}

diff_files $out_def [file join $tests_dir "parser_test.defok"]
exit 0
