# rebuffer 2 pin steiner with root branch other pt equal root
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def reg3.def
create_clock clk -period 1

set buffer_cell [get_lib_cell BUF_X2]
# kohm/micron, ff/micron
# use 100x wire cap to tickle buffer insertion
set_wire_rc -resistance 1.7e-4 -capacitance 13
sta::rebuffer_net [get_net u1z] $buffer_cell

set def_file [make_result_file rebuffer4.def]
write_def $def_file
diff_files rebuffer4.defok $def_file
