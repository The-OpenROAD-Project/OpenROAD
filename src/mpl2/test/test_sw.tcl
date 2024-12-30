# Case in which we treat the root as a macro cluster and jump
# from initializing the physical tree to macro placement
source "helpers.tcl"

read_lef "./SW/lef/asap7_tech_1x_201209.lef"
read_lef "./SW/lef/asap7sc7p5t_28_L_1x_220121a.lef"
read_lef "./SW/lef/asap7sc7p5t_28_R_1x_220121a.lef"
read_lef "./SW/lef/asap7sc7p5t_28_SL_1x_220121a.lef"
read_lef "./SW/lef/asap7sc7p5t_28_SRAM_1x_220121a.lef"
read_lef "./SW/lef/asap7sc7p5t_DFFHQNH2V2X.lef"
read_lef "./SW/lef/asap7sc7p5t_DFFHQNV2X.lef"
read_lef "./SW/lef/asap7sc7p5t_DFFHQNV4X.lef"
read_lef "./SW/lef/asap7sc7p5t_DFHV2X.lef"
read_lef "./SW/lef/fakeram7_64x21.lef"
read_lef "./SW/lef/fakeram7_256x34.lef"
read_lef "./SW/lef/fakeram7_2048x39.lef"

read_liberty "./SW/lib/asap7sc7p5t_AO_RVT_FF_ccs_211120.lib"
read_liberty "./SW/lib/asap7sc7p5t_INVBUF_RVT_FF_ccs_220122.lib"
read_liberty "./SW/lib/asap7sc7p5t_SIMPLE_RVT_FF_ccs_211120.lib"
read_liberty "./SW/lib/asap7sc7p5t_OA_RVT_FF_ccs_211120.lib"
read_liberty "./SW/lib/asap7sc7p5t_SEQ_RVT_FF_ccs_220123.lib"
read_liberty "./SW/lib/fakeram7_64x21.lib"
read_liberty "./SW/lib/fakeram7_256x34.lib"
read_liberty "./SW/lib/fakeram7_2048x39.lib"

put "reading verilog"
read_verilog "./SW/1.v"
put "done reading verilog"
link_design "swerv_wrapper"
read_sdc "./SW/1.sdc"
put "reading def"
read_def -floorplan_initialize "./SW/1_mpl2.def"
put "done reading def"

write_db sw_io.odb
