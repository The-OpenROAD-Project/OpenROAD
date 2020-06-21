# repair_long_wires -max_length 2 loads in L shape wire
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
# zero estimated parasitics to output ports
sta::set_pi_model u3/Z 0 0 0
sta::set_elmore u3/Z out1 0
sta::set_pi_model u4/Z 0 0 0
sta::set_elmore u4/Z out1 0

report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out2

report_long_wires 4

set buffer_cell [get_lib_cell BUF_X1]

repair_long_wires -max_length 1000 -buffer_cell $buffer_cell

report_long_wires 6
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out2
