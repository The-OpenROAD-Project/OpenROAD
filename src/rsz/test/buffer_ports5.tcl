# read_lef before read_liberty
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def reg2.def
create_clock -period 1 {clk1 clk2 clk3}

buffer_ports -inputs -outputs
