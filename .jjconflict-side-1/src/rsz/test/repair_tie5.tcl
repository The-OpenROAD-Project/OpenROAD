# repair tie to pad outside core
source "resizer_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
# intentionally skip read_liberty pad.lib
read_lef Nangate45/Nangate45.lef
read_lef pad.lef
read_def repair_tie5.def

repair_tie_fanout LOGIC1_X1/Z
check_ties LOGIC1_X1
report_ties LOGIC1_X1
check_in_core
