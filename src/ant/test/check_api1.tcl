# check_antennas tcl api commands
read_lef merged_spacing.lef
read_def sw130_random.def

check_antennas
puts "violation count = [ant::antenna_violation_count]"

# calculate maximum length allowed for routing levels 2,3 and 4
# for net50 while keeping the PAR ratios satisfied.
ant::find_max_allowed_length "net50" "met1"
ant::find_max_allowed_length "net50" "met2"
ant::find_max_allowed_length "net50" "met3"


# check if net50 has a violation
set net "net50"
puts "Net $net violations: [ant::check_net_violation $net]"
