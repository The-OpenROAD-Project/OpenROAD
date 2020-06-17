# repair_long_wires -max_length 2 loads
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
set max_wire_length [sta::find_max_wire_length $buffer_cell]
puts "max wire length [sta::format_distance $max_wire_length 0]"

set max_slew 200e-12
set max_slew_wire_length [sta::find_max_slew_wire_length $max_slew $buffer_cell]
puts "max slew wire length [sta::format_distance $max_slew_wire_length 0]"

repair_long_wires -max_length 1000 -buffer_cell $buffer_cell

report_long_wires 6
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out2
