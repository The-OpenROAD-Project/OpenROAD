# repair_design long wire wire to pad
source "resizer_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty Nangate45/fakeram45_64x32.lib
read_liberty pad.lib
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fakeram45_64x32.lef
read_lef pad.lef
read_def repair_wire5.def

set_wire_rc -layer metal3
estimate_parasitics -placement
report_check_types -max_slew -max_cap -max_fanout -violators

repair_design -max_wire_length 600
report_check_types -max_slew -max_cap -max_fanout -violators
check_in_core
