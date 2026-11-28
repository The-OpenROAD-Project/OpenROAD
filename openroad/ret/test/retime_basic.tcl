# Forward retiming of a plain register chain on Nangate45.
source "helpers.tcl"
source "ret_report.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 0.13 clk

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4

report_new_cells
