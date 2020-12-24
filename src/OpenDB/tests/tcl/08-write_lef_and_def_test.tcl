set tcl_dir [file dirname [file normalize [info script]]]
set tests_dir [file dirname $tcl_dir]
set data_dir [file join $tests_dir "data"]
set opendb_dir [file dirname $tests_dir]
source [file join $tcl_dir "test_helpers.tcl"]

set db [odb::dbDatabase_create]
set lib [odb::read_lef $db [file join $data_dir "gscl45nm.lef"]]
odb::read_def $db [file join $data_dir "design.def"]
set chip [$db getChip]
set block [$chip getBlock]
set lef_write_result [odb::write_lef $lib [file join $opendb_dir "build" "test.lef"]]
if {$lef_write_result != 1} {
    exit 1
}
set def_write_result [odb::write_def $block [file join $opendb_dir "build" "test.def"]]
if {$def_write_result != 1} {
    exit 1
}
exit 0
