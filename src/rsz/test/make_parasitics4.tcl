# estimate_parasitics input/output pads
read_liberty Nangate45/Nangate45_typ.lib
read_liberty pad.lib
read_lef Nangate45/Nangate45.lef
read_lef pad.lef
read_def make_parasitics4.def

set_wire_rc -layer metal3
estimate_parasitics -placement
report_net -connections -verbose in1
report_net -connections -verbose out1
report_checks -unconstrained -from in1
