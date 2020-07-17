# repair tie hi/low net
source "helpers.tcl"
source "resizer_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty repair_tie7.lib
read_lef Nangate45/Nangate45.lef
read_lef repair_tie7.lef
read_def repair_tie7.def

repair_tie_fanout TIE_X1/Z0
repair_tie_fanout TIE_X1/Z1
check_ties TIE_X1
report_ties TIE_X1
check_in_core
