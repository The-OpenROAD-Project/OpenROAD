# Writes macro placement of fake_macros.def
source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/fake_macros.lef
read_def fake_macros.def

set tcl_file [make_result_file write_macro_placement.tcl]

write_macro_placement $tcl_file

diff_file write_macro_placement.tclok $tcl_file