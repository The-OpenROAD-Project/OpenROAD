# repair_clock_nets 1 wire
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire1.def
create_clock in1 -period 10

set_wire_rc -layer metal3
estimate_parasitics -placement
# zero estimated parasitics to output port
sta::set_pi_model u3/Z 0 0 0
sta::set_elmore u3/Z out1 0

# wire length = 1500u -> 2 buffers required
repair_clock_nets -max_wire_length 600 -buffer_cell BUF_X1

report_long_wires 4
report_checks -unconstrained -fields {input slew cap} -digits 3 -rise_to out1
