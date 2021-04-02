# repair_design 1 wire
# in1--u1--u2--------u3-out1
#             2000u
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire1.def

set_wire_rc -layer metal3
estimate_parasitics -placement

set max_slew [rsz::default_max_slew]
puts "Max slew [sta::format_time $max_slew 3]ns"
set max_length [rsz::find_max_slew_wire_length [get_lib_pin BUF_X1/Z] [get_lib_pin BUF_X1/A] $max_slew]
puts "Max wire length [sta::format_distance $max_length 0]u"

# zero estimated parasitics to output port
set_load 0 [get_net out1]

report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
report_long_wires 2

# wire length = 2000u -> 2 buffers required
repair_design -max_wire_length 800

report_long_wires 4
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
