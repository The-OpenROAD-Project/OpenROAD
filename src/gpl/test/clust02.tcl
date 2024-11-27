source helpers.tcl
set test_name clust02


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


cluster_flops -tray_weight 60.0 \
              -timing_weight 1.0 \
              -max_split_size 50 \
              -num_paths 0
      
set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok
