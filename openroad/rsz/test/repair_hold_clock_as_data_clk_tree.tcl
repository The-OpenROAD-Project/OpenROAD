# repair_timing -hold must not insert hold buffers on clock-tree nets when it
# repairs a clock-as-data (cross-clock-domain) endpoint (OpenROAD #8075).
#
# clk1 drives the clock tree of r1 (clk1 -> i1 -> r1/CK) and is also used as
# data into r2/D (clk1 -> dbuf -> r2/D, r2 clocked by clk2).  r1 launches a
# cross-domain path into r3/D (clocked by clk2), so the clock-tree pin i1/A
# also has negative hold slack.  The r2/D hold path starts at the clk1 port,
# so the clk1 net is a candidate insertion point; buffering it would delay
# the clock of r1.  Hold buffers must only go on the data side (dbuf/Z).
source helpers.tcl
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_hold_clock_as_data_clk_tree.def

create_clock -period 2 -name clk1 [get_ports clk1]
create_clock -period 2 -name clk2 [get_ports clk2]
set_propagated_clock [all_clocks]
set_clock_latency 0.6 [get_clocks clk2]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal1
estimate_parasitics -placement

proc net_pins { net_name } {
  set names {}
  foreach pin [get_pins -of_objects [get_nets $net_name]] {
    lappend names [get_full_name $pin]
  }
  foreach port [get_ports -quiet -of_objects [get_nets $net_name]] {
    lappend names [get_full_name $port]
  }
  return [lsort $names]
}

proc report_clock_tree { } {
  puts "clk1 net: [net_pins clk1]"
  puts "i1z net: [net_pins i1z]"
  foreach prop {arrival_max_rise arrival_min_rise} {
    puts "r1/CK $prop: [get_property [get_pins r1/CK] $prop]"
  }
  puts "i1/A hold slack: [get_property [get_pins i1/A] slack_min]"
}

puts "=== before repair ==="
report_clock_tree
report_slack r2/D
report_slack r3/D

repair_timing -hold

puts "=== after repair ==="
report_clock_tree
report_slack r2/D
report_slack r3/D
