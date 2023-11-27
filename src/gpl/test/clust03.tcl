source helpers.tcl
set test_name clust03

read_lef ./asap7/asap7_tech_1x_201209.lef
read_lef ./asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef ./TwoBitTray/asap7sc7p5t_DFFHQNV2X.lef
read_lib ./TwoBitTray/asap7sc7p5t_DFFHQNV2X_RVT_TT_nldm_FAKE.lib
read_lib ./TwoBitTray/asap7sc7p5t_DFFHQNV2X_LVT_TT_nldm_FAKE.lib
read_lib ./TwoBitTray/asap7sc7p5t_DFFHQNV2X_SLVT_TT_nldm_FAKE.lib


read_lib ./clust.lib
read_lef ./clust.lef
read_def ./$test_name.def

cluster_flops -tray_weight 20.0 \
				-timing_weight 1.0 \
				-max_split_size 500

set def_file [make_result_file clust_sol03.def]
write_def $def_file
diff_file $def_file clust_sol03.defok
