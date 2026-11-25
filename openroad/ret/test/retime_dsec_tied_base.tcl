# A library without a plain flip-flop (ihp-sg13g2): the base register is the
# reset flip-flop with its reset tied inactive.  The result is proven
# sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib
read_lef ihp-sg13g2/sg13g2_tech.lef
read_lef ihp-sg13g2/sg13g2_stdcell.lef
read_verilog retime_dsec_tied_base.v
link_design top
create_clock -period 0.4 clk

write_verilog_for_dsec retime_dsec_tied_base before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
run_dsec_test retime_dsec_tied_base \
  -liberty_files ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib \
  -clock clk -resetn rst_n

set v_file [make_result_file retime_dsec_tied_base.v]
write_verilog $v_file
diff_files retime_dsec_tied_base.vok $v_file
