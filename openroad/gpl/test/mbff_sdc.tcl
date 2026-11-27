# cluster_flops keeps the SDC constraints on each flop D/QN pin on its
# tray pin and warns about the dropped CLK pin and instance constraints
source helpers.tcl

read_lef ./asap7/asap7_tech_1x_201209.lef
read_lef ./SingleBit/asap7sc7p5t_28_L_1x_220121a.lef
read_liberty ./SingleBit/asap7sc7p5t_SEQ_LVT_TT_nldm_220123.lib
read_lef ./4BitTrayH4/asap7sc7p5t_DFFHQNV4X.lef
read_liberty ./4BitTrayH4/asap7sc7p5t_DFFHQNV4X_LVT_TT_nldm_FAKE.lib

read_verilog ./mbff_orig_name.v
link_design tray_test

initialize_floorplan -die_area "0 0 10 10" \
  -core_area "1 1 9 9" \
  -site asap7sc7p5t

place_inst -name ff1 -origin {6 6} -status PLACED
place_inst -name ff2 -origin {4 6} -status PLACED
place_inst -name ff3 -origin {4 4} -status PLACED
place_inst -name ff4 -origin {6 4} -status PLACED

create_clock -name clk -period 1000 [get_ports clk1]
set_false_path -to [get_pins ff1/D]
set_multicycle_path 2 -setup -from [get_ports d2] -to [get_pins ff2/D]
set_max_delay 500 -through [get_pins ff3/QN] -to [get_ports o3]
set_case_analysis 0 [get_pins ff4/D]
set_max_capacitance 0.1 [get_pins ff2/QN]
set_min_pulse_width 30 [get_pins ff3/D]
set_clock_latency 5 [get_pins ff1/CLK]
set_false_path -from [get_ports d2] -to [get_cells ff1]
set_disable_timing [get_timing_edges -from [get_ports d1] -to [get_pins ff1/D]]

cluster_flops -tray_weight 40.0 \
  -timing_weight 1.0 \
  -max_split_size -1 \
  -num_paths 0

set sdc_file [make_result_file mbff_sdc.sdc]
write_sdc -no_timestamp $sdc_file
report_file $sdc_file
