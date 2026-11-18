# buffer input/output ports
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def buffer_ports1.def
create_clock -period 1 {clk1 clk2 clk3}

buffer_ports -inputs -outputs

set def_file [make_result_file buffer_ports1.def]
write_def $def_file
diff_files buffer_ports1.defok $def_file
