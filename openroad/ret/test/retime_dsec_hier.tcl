# A hierarchical netlist: registers merge and slide inside one module and
# stay put at a module boundary, both a register that feeds another module
# and one that reads its own module's port: 21 pinned gates, where a flat
# link has 18.  Two levels down, the inverting registers of each leaf are
# unmapped to plain registers plus inverters created in that leaf, and merge
# there.  The module nets are checked, and the result is proven sequentially
# equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
source "ret_report.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_hier.v
link_design -hier top
create_clock -period 0.15 clk

write_verilog_for_dsec retime_dsec_hier before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
check_axioms
run_dsec_test retime_dsec_hier -liberty_files Nangate45/Nangate45_typ.lib \
  -clock clk -resetn rst_n

report_new_cells
