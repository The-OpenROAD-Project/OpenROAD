# Unmapping two levels down a hierarchical netlist: the inverting registers
# of each leaf module become plain registers plus inverters created in that
# leaf.  The module nets are checked, and the result is proven sequentially
# equivalent with ABC dsec when EQUIVALENCE_CHECK is set.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_hier_unmap_nested.v
link_design -hier top
create_clock -period 1.0 clk

write_verilog_for_dsec retime_dsec_hier_unmap_nested before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
check_axioms
run_dsec_test retime_dsec_hier_unmap_nested \
  -liberty_files Nangate45/Nangate45_typ.lib -clock clk -resetn rst_n

set v_file [make_result_file retime_dsec_hier_unmap_nested.v]
write_verilog $v_file
diff_files retime_dsec_hier_unmap_nested.vok $v_file
