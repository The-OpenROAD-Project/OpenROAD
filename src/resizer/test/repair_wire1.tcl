# repair_design 1 wire
# in1--u1--u2--------u3-out1
#             1500u
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire1.def

set_wire_rc -layer metal3
estimate_parasitics -placement

set max_slew [sta::default_max_slew]
puts "Max slew [sta::format_time $max_slew 3]ns"
set max_length [sta::find_max_slew_wire_length $max_slew [get_lib_cell BUF_X1]]
puts "Max wire length [sta::format_distance $max_length 0]u"

# zero estimated parasitics to output port
sta::set_pi_model u3/Z 0 0 0
sta::set_elmore u3/Z out1 0

report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
report_long_wires 2

# wire length = 1500u -> 2 buffers required
repair_design -max_wire_length 600 -buffer_cell BUF_X1

report_long_wires 4
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
