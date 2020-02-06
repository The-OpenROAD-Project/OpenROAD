# rebuffer 2 pin steiner with root branch other pt equal root
source "helpers.tcl"
read_liberty liberty1.lib
read_lef liberty1.lef
read_def reg3.def
create_clock clk -period 1

set buffer_cell [get_lib_cell liberty1/snl_bufx2]
# kohm/micron, pf/micron
# use 100x wire cap to tickle buffer insertion
set_wire_rc -resistance 1.7e-4 -capacitance 1.3e-2
sta::rebuffer_net [get_net u1z] $buffer_cell

set def_file [make_result_file rebuffer4.def]
write_def $def_file
diff_files $def_file rebuffer4.defok
