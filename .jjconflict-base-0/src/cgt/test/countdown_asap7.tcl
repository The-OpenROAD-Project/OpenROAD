include "helpers.tcl"
read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_verilog countdown_asap7.v
link_design countdown
create_clock [get_ports clk] -name clock -period 0.5
clock_gating -min_instances 1
set verilog_file [make_result_file countdown_asap7_gated.v]
write_verilog $verilog_file
diff_file countdown_asap7_gated.vok $verilog_file
