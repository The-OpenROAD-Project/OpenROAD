# rebuffer input/output ports
source "helpers.tcl"
read_liberty liberty1.lib
read_lef liberty1.lef
read_def rebuffer_ports1.def
create_clock -period 1 {clk1 clk2 clk3}

set buffer_cell [get_lib_cell liberty1/snl_bufx2]
resize -buffer_inputs -buffer_cell $buffer_cell
resize -buffer_outputs -buffer_cell $buffer_cell

set def_file [make_result_file rebuffer_ports1.def]
write_def $def_file
diff_files $def_file rebuffer_ports1.defok
