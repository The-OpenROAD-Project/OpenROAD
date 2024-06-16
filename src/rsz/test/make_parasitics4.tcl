# estimate_parasitics input/output pads
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty pad.lib
read_lef Nangate45/Nangate45.lef
read_lef pad.lef
read_def make_parasitics4.def

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
report_net in1
report_net out1
report_checks -unconstrained -from in1
