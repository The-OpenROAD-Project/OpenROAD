source "helpers.tcl"
# Net with two disjoint li1 stubs bridged by pin u1/A: the gate record must
# merge the stubs as a union, independent of node visit order
read_lef ant_check.lef
read_def check_disjoint_stubs.def

check_antennas -verbose
puts "violation count = [ant::antenna_violation_count]"

set net "n1"
puts "Net $net violations: [ant::check_net_violation $net]"
