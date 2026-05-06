source helpers.tcl
set test_name mbff_orig_name

read_lef ./asap7/asap7_tech_1x_201209.lef
read_lef ./SingleBit/asap7sc7p5t_28_L_1x_220121a.lef
read_lib ./SingleBit/asap7sc7p5t_SEQ_LVT_TT_nldm_220123.lib

read_lef ./2BitTrayH2/asap7sc7p5t_DFFHQNV2X.lef
read_lib ./2BitTrayH2/asap7sc7p5t_DFFHQNV2X_LVT_TT_nldm_FAKE.lib

read_lef ./4BitTrayH4/asap7sc7p5t_DFFHQNV4X.lef
read_lib ./4BitTrayH4/asap7sc7p5t_DFFHQNV4X_LVT_TT_nldm_FAKE.lib

read_lef ./4BitTrayH2W2/asap7sc7p5t_DFFHQNH2V2X.lef
read_lib ./4BitTrayH2W2/asap7sc7p5t_DFFHQNH2V2X_LVT_TT_nldm_FAKE.lib

read_def ./$test_name.def

create_clock -name clk -period 1000 [get_ports clk1]
set_input_delay -clock clk 0 [get_ports {d1 d2 d3 d4}]

cluster_flops -tray_weight 40.0 \
  -timing_weight 1.0 \
  -max_split_size -1 \
  -num_paths 0

# Report timing to verify original FF names appear in the path report.
# After clustering the tray pin descriptions should show in the Orig Name column.
report_checks -path_delay max -fields {orig_name} -through [get_pins _tray_size4_7/D1]
