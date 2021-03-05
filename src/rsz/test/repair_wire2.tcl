# repair_design 2 loads in L shape wire
#
#                    u4-out2
#                    |
#                    |1500u
#                    |
# in1--u1--u2--------u3-out1
#             1500u
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire2.def

set_wire_rc -layer metal3
estimate_parasitics -placement
# zero estimated parasitics to output ports
set_load 0 [get_net out1]
set_load 0 [get_net out2]

report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out2

report_long_wires 4

repair_design -max_wire_length 1000

report_long_wires 6
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out2
