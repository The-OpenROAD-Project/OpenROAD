# retime_dsec_tied_base with the tie cells marked set_dont_use, as ORFS marks
# them on gt2n: the base register is still the reset flip-flop tied inactive.
# The result is proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib
read_lef ihp-sg13g2/sg13g2_tech.lef
read_lef ihp-sg13g2/sg13g2_stdcell.lef
read_verilog retime_dsec_tied_base.v
link_design top
create_clock -period 0.4 clk
set_dont_use {sg13g2_tiehi sg13g2_tielo}

write_verilog_for_dsec retime_dsec_dont_use_tie before
retime
run_dsec_test retime_dsec_dont_use_tie \
  -liberty_files ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib \
  -clock clk -resetn rst_n

set v_file [make_result_file retime_dsec_dont_use_tie.v]
write_verilog $v_file
diff_files retime_dsec_tied_base.vok $v_file
