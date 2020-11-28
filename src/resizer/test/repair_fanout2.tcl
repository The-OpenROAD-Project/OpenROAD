# repair_design max_fanout hands off special nets
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_fanout2.def
set_max_fanout 10 [current_design]
repair_design
