source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def write_pin_placement5.def

set tcl_file [make_result_file write_pin_placement6.tcl]
set def_file [make_result_file write_pin_placement6.def]

write_pin_placement $tcl_file

source $tcl_file

write_def $def_file

diff_file write_pin_placement6.defok $def_file

diff_file write_pin_placement6.tclok $tcl_file
