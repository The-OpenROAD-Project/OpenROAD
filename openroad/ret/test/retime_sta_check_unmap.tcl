# The STA check on a retiming that unmaps registers first: the search is
# repeated on the netlist with the unmapped registers, and the slides the check
# reports are those of the search whose result it applied (9, not the 7 of the
# first search).  Proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_existing_names.v
link_design top
create_clock -period 0.16 clk

write_verilog_for_dsec retime_sta_check_unmap before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
run_dsec_test retime_sta_check_unmap \
  -liberty_files Nangate45/Nangate45_typ.lib -clock clk -resetn rst_n
