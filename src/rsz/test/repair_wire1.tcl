# repair_design 1 wire
# in1--u1--u2--------u3-out1
#             2000u
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire1.def

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# zero estimated parasitics to output port
set_load 0 [get_net out1]

report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
report_long_wires 2

# wire length = 2000u -> 2 buffers required
repair_design -max_wire_length 800

report_long_wires 4
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
