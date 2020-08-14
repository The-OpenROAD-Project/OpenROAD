# repair_clock_nets 1 wire
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire1.def
create_clock in1 -period 10

set_wire_rc -layer metal3
estimate_parasitics -placement
set_load 0 [get_net out1]

foreach net_name {in1 n2 n2} {
  [sta::sta_to_db_net [get_net $net_name]] setSigType CLOCK
}

# wire length = 1500u -> 2 buffers required
repair_clock_nets -max_wire_length 600 -buffer_cell BUF_X1

puts [[sta::sta_to_db_net [get_net net2]] getSigType]

report_long_wires 4
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
