# repair_timing r1/Q 5 loads
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup1.def
create_clock -period 0.3 clk
set_wire_rc -layer metal3
estimate_parasitics -placement
report_checks -fields input -digits 3
report_power

repair_timing -setup
report_checks -fields input -digits 3
report_power
