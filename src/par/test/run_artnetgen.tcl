source "helpers.tcl"

set design jpeg_encoder 
set spec_dir .
set verilog_dir .

if { ![file exists $spec_dir] } {
    file mkdir $spec_dir
}

read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib
read_liberty asap7/asap7sc7p5t_AO_RVT_SS_nldm_211120.lib
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_SS_nldm_220122.lib
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib
read_liberty asap7/asap7sc7p5t_OA_RVT_SS_nldm_211120.lib
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_liberty asap7/asap7sc7p5t_SEQ_RVT_SS_nldm_220123.lib
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_SS_nldm_211120.lib


read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef

read_verilog ${design}.v
link_design ${design}

read_sdc ${design}.sdc

write_artnet_spec -out_file ${spec_dir}/${design}.spec

exit
