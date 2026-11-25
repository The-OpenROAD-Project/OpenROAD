# A register that drives only the enable pin of another register keeps its
# depth, so the gate in front of it is never crossed: the design fails timing
# and nothing moves.  The result is proven sequentially equivalent with ABC
# dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_dsec_enable_only.v
link_design top
create_clock -period 0.5 clk

write_verilog_for_dsec retime_dsec_enable_only before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
run_dsec_test retime_dsec_enable_only \
  -liberty_files sky130hd/sky130hd_tt.lib -clock clk -resetn rst_n

set v_file [make_result_file retime_dsec_enable_only.v]
write_verilog $v_file
diff_files retime_dsec_enable_only.vok $v_file
