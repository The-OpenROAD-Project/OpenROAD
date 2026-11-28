# Retime a netlist that already holds the instance and net names an earlier
# retime run generated: new cells and nets take names that are still free.
# The registers are unmapped first, so the search runs again on the unmapped
# netlist; the STA check reports that search's 9 slides, not the first
# search's 7.  The result is proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
source "ret_report.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_existing_names.v
link_design top
create_clock -period 0.16 clk

write_verilog_for_dsec retime_dsec_existing_names before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
run_dsec_test retime_dsec_existing_names \
  -liberty_files Nangate45/Nangate45_typ.lib -clock clk -resetn rst_n

report_new_cells
