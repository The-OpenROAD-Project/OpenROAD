set tcl_dir [file dirname [file normalize [info script]]]
set tests_dir [file dirname $tcl_dir]
set data_dir [file join $tests_dir "data"]
source [file join $tcl_dir "test_helpers.tcl"]

set db [odb::dbDatabase_create]
odb::read_lef $db [file join $data_dir "gscl45nm.lef"]
odb::read_def $db [file join $data_dir "design.def"]
if {[$db getChip] == "NULL"} {
    puts "Read DEF Failed"
    exit 1
}
exit 0
