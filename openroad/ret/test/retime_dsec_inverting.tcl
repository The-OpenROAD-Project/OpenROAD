# Inverting registers (a flop read through its inverted output) keep their
# cell across a self-dual gate (cone 1) and are unmapped before a gate that is
# not self-dual, such as a full adder with an input tied to 1'b0 (cone 2).
# The result is proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
source "ret_report.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_inverting.v
link_design top
create_clock -period 0.15 clk

write_verilog_for_dsec retime_dsec_inverting before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
run_dsec_test retime_dsec_inverting -liberty_files Nangate45/Nangate45_typ.lib \
  -clock clk -resetn rst_n

report_new_cells
