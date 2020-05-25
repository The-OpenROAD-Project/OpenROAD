# repair_max_fanout hands off special nets
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_max_fanout2.def
repair_max_fanout -max_fanout 10 -buffer_cell BUF_X1
