source "helpers.tcl"
read_lef merged_spacing.lef
read_def sw130_random.def

set rpt_file [make_result_file "sw130_random.rpt"]
check_antennas -report_file $rpt_file
report_file $rpt_file

# calculate the available length that can be added to net51, on layer 1
# while keeping the PAR ratios satified
ant::check_max_length "net51" 1

# check if net50 has a violation
set net "net50"
puts "Net $net violations: [ant::check_net_violation $net]"
