source helpers.tcl
set test_name mbff_orig_name_hier

read_lef ./asap7/asap7_tech_1x_201209.lef
read_lef ./SingleBit/asap7sc7p5t_28_L_1x_220121a.lef
read_lib ./SingleBit/asap7sc7p5t_SEQ_LVT_TT_nldm_220123.lib

read_lef ./2BitTrayH2/asap7sc7p5t_DFFHQNV2X.lef
read_lib ./2BitTrayH2/asap7sc7p5t_DFFHQNV2X_LVT_TT_nldm_FAKE.lib

read_lef ./4BitTrayH4/asap7sc7p5t_DFFHQNV4X.lef
read_lib ./4BitTrayH4/asap7sc7p5t_DFFHQNV4X_LVT_TT_nldm_FAKE.lib

read_lef ./4BitTrayH2W2/asap7sc7p5t_DFFHQNH2V2X.lef
read_lib ./4BitTrayH2W2/asap7sc7p5t_DFFHQNH2V2X_LVT_TT_nldm_FAKE.lib

read_def ./mbff_orig_name.def

# Move each ff into its own sub-module so dbInst::getModule() returns
# a non-top dbModule. This exercises the hierarchical-name path of
# MBFF::ModifyPinConnections — the orig_name property stored on the tray
# (and shown in the timing report) should include the full path,
# e.g. "u3/ff3/D" rather than just "ff3/D".
set block [ord::get_db_block]
set top_mod [$block getTopModule]
foreach idx {1 2 3 4} {
  set master_mod [odb::dbModule_create $block "sub_master_${idx}"]
  odb::dbModInst_create $top_mod $master_mod "u${idx}"
  $master_mod addInst [$block findInst "ff${idx}"]
}

create_clock -name clk -period 1000 [get_ports clk1]
set_input_delay -clock clk 0 [get_ports {d1 d2 d3 d4}]

cluster_flops -tray_weight 40.0 \
  -timing_weight 1.0 \
  -max_split_size -1 \
  -num_paths 0

# After clustering the tray pin description should show the hierarchical
# original FF pin name in the Orig Name column.
report_checks -path_delay max -fields {orig_name} -through [get_pins _tray_size4_7/D1]
