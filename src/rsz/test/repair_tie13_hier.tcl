# Test repair_tie_fanout with name collision in hierarchical flow
source "helpers.tcl"
source "resizer_helpers.tcl"

set test_name repair_tie13_hier

read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef

read_verilog $test_name.v
link_design -hier top

# This will create a dbModNet 'net_1' in HIER1 because there exists
# an internal dbNet 'net'.
repair_tie_fanout TIEHIx1_ASAP7_75t_R/H

set verilog_out [make_result_file $test_name.v]
write_verilog $verilog_out
diff_file $test_name.vok $verilog_out
