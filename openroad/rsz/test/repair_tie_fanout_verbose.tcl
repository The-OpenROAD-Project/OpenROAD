source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_tie1.def

repair_tie_fanout -verbose LOGIC1_X1/Z
