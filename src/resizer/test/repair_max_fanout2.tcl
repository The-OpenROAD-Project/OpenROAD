# repair_max_fanout hands off special nets
source helpers.tcl
read_liberty Nangate_typ.lib
read_lef Nangate.lef
read_def repair_max_fanout2.def
repair_max_fanout -max_fanout 10 -buffer_cell BUF_X1
