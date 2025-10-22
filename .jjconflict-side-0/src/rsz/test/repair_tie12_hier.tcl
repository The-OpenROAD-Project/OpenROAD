# Test repair_tie_fanout in a hierarchical flow
source "helpers.tcl"
source "resizer_helpers.tcl"

set test_name repair_tie12_hier

read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
#read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
#read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
#read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef

read_verilog $test_name.v
link_design -hier top

report_net hier1/MI0

#[[[ord::get_db] getChip] getBlock] debugPrintContent

repair_tie_fanout TIEHIx1_ASAP7_75t_R/H

check_ties TIEHIx1_ASAP7_75t_R
report_ties TIEHIx1_ASAP7_75t_R

set verilog_file [make_result_file $test_name.v]
write_verilog $verilog_file
diff_files $test_name.vok $verilog_file

set def_file [make_result_file $test_name.def]
write_def $def_file
diff_files $test_name.defok $def_file
