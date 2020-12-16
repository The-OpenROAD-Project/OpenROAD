set tcl_dir [file dirname [file normalize [info script]]]
set tests_dir [file dirname $tcl_dir]
set data_dir [file join $tests_dir "data"]
source [file join $tcl_dir "test_helpers.tcl"]

set db [odb::dbDatabase_create]
odb::read_lef $db [file join $data_dir "gscl45nm.lef"]
odb::read_def $db [file join $data_dir "design.def"]
set chip [$db getChip]
set block [$chip getBlock]
set nets [$block getNets]
foreach net $nets {
    puts "Net: [$net getName]"
}
exit [expr [llength $nets] != 24]
