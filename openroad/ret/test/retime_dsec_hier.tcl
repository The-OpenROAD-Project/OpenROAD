# A hierarchical netlist: registers merge and slide inside one module and
# stay put at a module boundary.  The module nets are checked, and the
# result is proven sequentially equivalent with ABC dsec when
# EQUIVALENCE_CHECK is set.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_hier.v
link_design -hier top
create_clock -period 1.0 clk

write_verilog_for_dsec retime_dsec_hier before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
check_axioms
run_dsec_test retime_dsec_hier -liberty_files Nangate45/Nangate45_typ.lib \
  -clock clk -resetn rst_n

set v_file [make_result_file retime_dsec_hier.v]
write_verilog $v_file
diff_files retime_dsec_hier.vok $v_file
