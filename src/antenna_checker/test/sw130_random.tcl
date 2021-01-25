source "helpers.tcl"
read_lef merged_spacing.lef
read_def -order_wires sw130_random.def

# start checking antennas and generate a detail report
check_antennas -report_file [make_result_file "antenna.rpt"]

# calculate the available length that can be added to net51, on layer 1
# while keeping the PAR ratios satified
ant::check_max_length "net51" 1

# check if net50 has a violation
set net "net50"
puts "Net - $net has violation: [ant::check_net_violation $net]"
