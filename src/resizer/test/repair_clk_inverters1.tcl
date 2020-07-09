# repair_clock_inverters
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_clk_inverters1.def
create_clock -period 1 clk1
repair_clock_inverters
