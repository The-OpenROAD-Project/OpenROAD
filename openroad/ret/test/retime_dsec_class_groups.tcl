# Registers of different classes that share a data net: a register that moves
# keeps its cell, and the registers of the other classes on the net are neither
# rebuilt nor unmapped.  The result is proven sequentially equivalent with ABC
# dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
source "ret_report.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_dsec_class_groups.v
link_design top
create_clock -period 0.5 clk

write_verilog_for_dsec retime_dsec_class_groups before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
run_dsec_test retime_dsec_class_groups \
  -liberty_files sky130hd/sky130hd_tt.lib -clock clk -resetn rst_n

report_new_cells
