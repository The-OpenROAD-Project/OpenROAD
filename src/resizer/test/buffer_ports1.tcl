# buffer input/output ports
source "helpers.tcl"
read_liberty Nangate_typ.lib
read_lef Nangate.lef
read_def reg2.def
create_clock -period 1 {clk1 clk2 clk3}

set buffer_cell [get_lib_cell BUF_X2]
buffer_ports -inputs -outputs -buffer_cell $buffer_cell

set def_file [make_result_file buffer_ports1.def]
write_def $def_file
diff_files buffer_ports1.defok $def_file
