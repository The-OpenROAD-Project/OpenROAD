source "helpers.tcl"

define_corners fast slow
read_liberty -corner slow ./asap7/asap7sc7p5t_AO_RVT_SS_nldm_211120.lib.gz
read_liberty -corner slow ./asap7/asap7sc7p5t_INVBUF_RVT_SS_nldm_220122.lib.gz
read_liberty -corner slow ./asap7/asap7sc7p5t_OA_RVT_SS_nldm_211120.lib.gz
read_liberty -corner slow ./asap7/asap7sc7p5t_SEQ_RVT_SS_nldm_220123.lib
read_liberty -corner slow ./asap7/asap7sc7p5t_SIMPLE_RVT_SS_nldm_211120.lib.gz
read_liberty -corner fast ./asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty -corner fast ./asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty -corner fast ./asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty -corner fast ./asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_liberty -corner fast ./asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz

read_lef ./asap7/asap7_tech_1x_201209.lef
read_lef ./asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_verilog ./gcd_asap7.v
link_design gcd
read_sdc ./gcd_asap7.sdc


puts "-- Before --\n"
report_cell_usage
report_timing_histogram
report_checks
report_wns
report_tns

puts "-- After --\n"

resynth -corner fast
report_timing_histogram
report_cell_usage
report_checks
report_wns
report_tns

resynth -corner fast
report_timing_histogram
report_cell_usage
report_checks
report_wns
report_tns
