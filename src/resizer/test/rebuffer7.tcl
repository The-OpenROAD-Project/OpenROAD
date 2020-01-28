# rebuffer reg -> reg and output port (no insertion)
source "helpers.tcl"
read_liberty liberty1.lib
read_lef liberty1.lef
read_def rebuffer7.def
create_clock clk -period 1

set buffer_cell [get_lib_cell liberty1/snl_bufx2]
# use 100x wire cap to tickle buffer insertion
set_wire_rc -resistance 1.7e+5 -capacitance 1.3e-8

sta::rebuffer_net [get_net out] $buffer_cell

set def_file [make_result_file rebuffer7.def]
write_def $def_file
diff_files $def_file rebuffer7.defok
