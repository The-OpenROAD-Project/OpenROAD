# repair_clock_nets 1 wire
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_wire1.def
create_clock in1 -period 10

# If set_propagated_clock setting is omitted, `in1` is assumed to be
# an ideal clock.
# `estimate_parasitics -placement` does not compute RC for ideal clock nets.
# Therefore, executing set_propagated_clock is crucial for calculating
# an accurate RC.
set_propagated_clock in1

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

puts "\n# Set CLOCK type for all nets in the target path."
foreach net_name {in1 n1 n2 out1} {
  [sta::sta_to_db_net [get_net $net_name]] setSigType CLOCK
}

# wire length = 1500u -> at least 2 buffers are required
repair_clock_nets -max_wire_length 600

puts "\n# All nets in the path should be CLOCK type."
foreach pin_name {u1/A u1/Z u2/Z wire3/Z wire2/Z wire1/Z u3/Z} {
  set net [get_net -of_object [get_pins $pin_name]]
  set net_name [get_name $net]
  puts "$net_name: [[sta::sta_to_db_net $net] getSigType]"
}

report_long_wires 4
report_checks -unconstrained -fields {input slew cap} -rise_to out1

proc location { inst_name args } {
  set inst [[ord::get_db_block] findInst $inst_name]
  puts "Instance: $inst_name, Location (unit: DBU): [$inst getLocation]"
}

puts "\n# Check new buffer locations."
location u2
location wire3
location wire2
location wire1
location u3

puts "\n# Call sta::network_changed. Timing should be the same."
estimate_parasitics -placement
report_checks -unconstrained -fields {input slew cap} -rise_to out1

puts "\n# All nets in the path should be CLOCK type after network_changed."
foreach pin_name {u1/A u1/Z u2/Z wire3/Z wire2/Z wire1/Z u3/Z} {
  set net [get_net -of_object [get_pins $pin_name]]
  set net_name [get_name $net]
  puts "$net_name: [[sta::sta_to_db_net $net] getSigType]"
}
