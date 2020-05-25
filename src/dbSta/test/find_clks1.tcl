# find_clks from input port thru pad
read_lef Nangate45/Nangate45.lef
read_lef pad.lef
read_liberty Nangate45/Nangate45_typ.lib
read_liberty pad.lib
read_def find_clks1.def

create_clock -name clk -period 10 clk1
sta::report_clk_nets
