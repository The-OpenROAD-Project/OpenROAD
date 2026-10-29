# cluster_flops keeps the activity annotation of each flop pin on its tray pin
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

set_power_activity -pins [get_pins ff1/QN] -activity 0.01
set_power_activity -pins [get_pins ff2/QN] -activity 0.02
set_power_activity -pins [get_pins ff3/QN] -activity 0.03
set_power_activity -pins [get_pins ff4/QN] -activity 0.04
set_power_activity -pins [get_pins ff1/D] -activity 0.05
set_power_activity -pins [get_pins ff2/D] -activity 0.06
set_power_activity -pins [get_pins ff3/D] -activity 0.07
set_power_activity -pins [get_pins ff4/D] -activity 0.08

cluster_flops -tray_weight 40.0 \
  -timing_weight 1.0 \
  -max_split_size -1 \
  -num_paths 0

report_activity_annotation -report_annotated
foreach pin [get_pins {_tray_size4_*/D* _tray_size4_*/QN*}] {
  puts "[get_full_name $pin] [get_property $pin activity]"
}
