source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def write_pin_placement5.def

set tcl_file [make_result_file write_pin_placement5.tcl]
set def_file [make_result_file write_pin_placement5.def]

write_pin_placement $tcl_file -placed_status

source $tcl_file

write_def $def_file

diff_file write_pin_placement5.defok $def_file

diff_file write_pin_placement5.tclok $tcl_file
