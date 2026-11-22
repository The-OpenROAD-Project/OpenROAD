# The STA check on a retiming that unmaps registers first: the search is
# repeated on the netlist with the unmapped registers, and the slides the
# check reports are those of the search whose result it applied (10, not the
# 8 of the first search; see retime_dsec_existing_names).  Proven
# sequentially equivalent with ABC dsec when EQUIVALENCE_CHECK is set.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_existing_names.v
link_design top
create_clock -period 1.0 clk

write_verilog_for_dsec retime_sta_check_unmap before
report_worst_slack -max -digits 4
set_debug_level RET sta_check 1
retime
report_worst_slack -max -digits 4
run_dsec_test retime_sta_check_unmap \
  -liberty_files Nangate45/Nangate45_typ.lib -clock clk -resetn rst_n
