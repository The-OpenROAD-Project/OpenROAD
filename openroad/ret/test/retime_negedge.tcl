# A negative-edge register is left in place with a warning while the
# positive-edge registers retime. The result is proven sequentially equivalent
# with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
set libs [list \
  asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz \
  asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz \
  asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib]
foreach lib $libs {
  read_liberty $lib
}
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_verilog retime_negedge.v
link_design top
create_clock -period 80 clk

write_verilog_for_dsec retime_negedge before
report_worst_slack -max -digits 2
retime
report_worst_slack -max -digits 2
run_dsec_test retime_negedge -liberty_files $libs -clock clk -resetn rst_n

set v_file [make_result_file retime_negedge.v]
write_verilog $v_file
diff_files retime_negedge.vok $v_file
