# A chain of a plain and an enable register that only loses its last register:
# the register left behind keeps its own class.  The result is proven
# sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_dsec_enable_chain.v
link_design top
create_clock -period 0.6 clk

write_verilog_for_dsec retime_dsec_enable_chain before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
run_dsec_test retime_dsec_enable_chain \
  -liberty_files sky130hd/sky130hd_tt.lib -clock clk -resetn rst_n

set v_file [make_result_file retime_dsec_enable_chain.v]
write_verilog $v_file
diff_files retime_dsec_enable_chain.vok $v_file
