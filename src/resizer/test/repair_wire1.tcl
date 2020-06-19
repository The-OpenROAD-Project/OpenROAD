# repair_long_wires 1 wire
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire1.def

set_wire_rc -layer metal3
# zero estimated parasitics to output port
sta::set_pi_model u3/Z 0 0 0
sta::set_elmore u3/Z out1 0

report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
report_long_wires 2

set buffer_cell [get_lib_cell BUF_X1]
# wire length = 1500u
repair_long_wires -max_length 600 -buffer_cell $buffer_cell

report_long_wires 4
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
