source "helpers.tcl"
# check_antennas tcl api commands
read_lef merged_spacing.lef
read_def sw130_random.def

check_antennas -verbose
puts "violation count = [ant::antenna_violation_count]"

# check if net50 has a violation
set net "net50"
puts "Net $net violations: [ant::check_net_violation $net]"
