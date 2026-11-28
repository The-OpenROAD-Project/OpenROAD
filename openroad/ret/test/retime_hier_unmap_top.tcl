# The design of retime_dsec_enable linked hierarchically: every register is in
# the top module.  The mixed-class registers r4 and r6 are unmapped there, as
# in the flat link, and the same-class merge of r1 and r2 keeps its enable
# cell.  Its new cells, all in the top module, are those of
# retime_dsec_enable, which proves them sequentially equivalent with ABC
# dsec.
source "helpers.tcl"
source "ret_report.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_dsec_enable.v
link_design -hier top
create_clock -period 0.75 clk

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
check_axioms

report_new_cells
