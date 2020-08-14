source "helpers.tcl"
read_lef merged_spacing.lef
read_def -order_wires sw130_random.def

# load layers' antenna rules into ARC
load_antenna_rules

# start checking antennas and generate a detail report
check_antennas -path [make_result_file "./"]

# calculate the available length that can be added to net51, at route level 1, while keeping the PAR ratios satified
get_met_avail_length -net_name "net51" -route_level 1

# check if net50 has a violation
set target_net "net50"
set vio [check_net_violation -net_name $target_net]
puts "Net - $target_net has violation: $vio"

# set the antenna diode name
set antenna_cell_name "sky130_fd_sc_ms__diode_2"
# set the diode pin for connection
set pin_name "DIODE"
# set the output file name
set target_file "design_with_diodes"
antenna_fix $antenna_cell_name $pin_name

set verilog_file_name [make_result_file "$target_file.v"]
write_verilog  $verilog_file_name

set def_file_name [make_result_file "$target_file.def"]
write_def $def_file_name

