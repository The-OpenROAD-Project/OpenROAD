source "helpers.tcl"
source "resizer_helpers.tcl"

read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef

read_verilog remove_buffers_hier1.v
link_design -hier top
set_max_delay -from i -to k 10

puts "Before removal:"
report_edges -to m1/gate1/B
report_checks -from i -to k

remove_buffers m1/buf1

puts "After removal:"
report_edges -to m1/gate1/B
report_checks -from i -to k
