# The tied-down base register inside a module of a hierarchical netlist
# (ihp-sg13g2): new registers and their tie cells are created in the module,
# and a tie cell left without a load is removed.  The module nets are checked,
# and the result is proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib
read_lef ihp-sg13g2/sg13g2_tech.lef
read_lef ihp-sg13g2/sg13g2_stdcell.lef
read_verilog retime_dsec_hier_tied_base.v
link_design -hier top
create_clock -period 0.4 clk

write_verilog_for_dsec retime_dsec_hier_tied_base before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
check_axioms
run_dsec_test retime_dsec_hier_tied_base \
  -liberty_files ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib \
  -clock clk -resetn rst_n

set v_file [make_result_file retime_dsec_hier_tied_base.v]
write_verilog $v_file
diff_files retime_dsec_hier_tied_base.vok $v_file
