# Test repair_tie_fanout on a hierarchical asap7 gcd2

source "helpers.tcl"
source "resizer_helpers.tcl"

read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef

read_verilog repair_tie10_hier.v
link_design -hier gcd

report_net _395_/BI
report_net _395_/CI

puts "Execute repair_tie_fanout."
repair_tie_fanout TIEHIx1_ASAP7_75t_R/H

report_net _395_/BI
report_net _395_/CI

check_ties TIEHIx1_ASAP7_75t_R
report_ties TIEHIx1_ASAP7_75t_R

sta::check_axioms
