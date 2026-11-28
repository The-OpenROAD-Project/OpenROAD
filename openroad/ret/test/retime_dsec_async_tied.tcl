# Registers whose asynchronous reset is tied inactive move like registers
# without one, and the tie cells that only they read are removed: r1 and r2
# merge into one DFF_X1, and r4, read through QN, is unmapped to a DFF_X1
# read through Q plus an inverter.  r6, with a live reset, stays.  The
# result is proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
source "ret_report.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_async_tied.v
link_design top
create_clock -period 0.15 clk

write_verilog_for_dsec retime_dsec_async_tied before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
report_kept {r1 r2 t1 t2 r4 t4 r6}
report_new_cells
run_dsec_test retime_dsec_async_tied \
  -liberty_files Nangate45/Nangate45_typ.lib -clock clk -resetn rst_n
