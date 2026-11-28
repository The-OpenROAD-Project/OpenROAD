# A register slides forward inside a feedback loop; the loop keeps its register
# count.  The result is proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
source "ret_report.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_loop.v
link_design top
create_clock -period 0.2 clk

write_verilog_for_dsec retime_dsec_loop before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
run_dsec_test retime_dsec_loop -liberty_files Nangate45/Nangate45_typ.lib \
  -clock clk -resetn rst_n

report_new_cells
