source "helpers.tcl"
# A minimal LEF file that has been modified to include particular antenna values for testing
read_lef ant_check.lef
read_def ant_check.def

check_antennas -verbose
puts "violation count = [ant::antenna_violation_count]"

# check if net50 has a violation
set net "net50"
puts "Net $net violations: [ant::check_net_violation $net]"
