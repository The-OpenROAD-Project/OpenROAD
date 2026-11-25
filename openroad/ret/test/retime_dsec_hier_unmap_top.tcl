# The design of retime_dsec_enable linked hierarchically: every register is in
# the top module.  The mixed-class registers r4 and r6 are unmapped there, as
# in the flat link, and the same-class merge of r1 and r2 keeps its enable
# cell.  The module nets are checked, and the result is proven sequentially
# equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_dsec_enable.v
link_design -hier top
create_clock -period 0.75 clk

write_verilog_for_dsec retime_dsec_hier_unmap_top before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
check_axioms
run_dsec_test retime_dsec_hier_unmap_top \
  -liberty_files sky130hd/sky130hd_tt.lib -clock clk -resetn rst_n

set v_file [make_result_file retime_dsec_hier_unmap_top.v]
write_verilog $v_file
diff_files retime_dsec_hier_unmap_top.vok $v_file
