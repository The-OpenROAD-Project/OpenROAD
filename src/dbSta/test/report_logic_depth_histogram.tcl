# report_logic_depth_histogram
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_liberty "Nangate45/Nangate45_typ.lib"
read_def report_logic_depth_histogram.def

create_clock -name clk -period 1 {clk}
set_input_delay -clock clk 0.25 {in1 in2}
set_output_delay -clock clk 0.25 {out1 out2}

puts "report_logic_depth_histogram -num_bins 5"
report_logic_depth_histogram -num_bins 5

puts "report_logic_depth_histogram -num_bins 5 -exclude_buffers"
report_logic_depth_histogram -num_bins 5 -exclude_buffers

puts "report_logic_depth_histogram -num_bins 5 -exclude_inverters"
report_logic_depth_histogram -num_bins 5 -exclude_inverters

puts "report_logic_depth_histogram -num_bins 5 -exclude_buffers -exclude_inverters"
report_logic_depth_histogram -num_bins 5 -exclude_buffers -exclude_inverters
