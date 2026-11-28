# Unmapping inside a module of a hierarchical netlist: the mixed-class
# registers r4 and r6 of `core` become plain registers plus logic created in
# `core`, and then merge.  The module nets are checked, and the result is
# proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
source "ret_report.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_dsec_hier_unmap.v
link_design -hier top
create_clock -period 1.0 clk

write_verilog_for_dsec retime_dsec_hier_unmap before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
check_axioms
run_dsec_test retime_dsec_hier_unmap -liberty_files sky130hd/sky130hd_tt.lib \
  -clock clk -resetn rst_n

report_new_cells
