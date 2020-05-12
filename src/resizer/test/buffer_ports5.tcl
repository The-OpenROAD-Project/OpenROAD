# read_lef before read_liberty
read_lef Nangate.lef
read_liberty Nangate_typ.lib
read_def reg2.def
create_clock -period 1 {clk1 clk2 clk3}

set buffer_cell [get_lib_cell BUF_X2]
buffer_ports -inputs -outputs -buffer_cell $buffer_cell
