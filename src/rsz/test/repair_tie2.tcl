# repair_tie_fanout -separation
source "helpers.tcl"
source "resizer_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
# This is not generated because it has mirrored instances
read_def repair_tie2.def

repair_tie_fanout -separation 2 LOGIC1_X1/Z
check_ties LOGIC1_X1
report_ties LOGIC1_X1
check_in_core
