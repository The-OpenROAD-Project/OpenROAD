# A primary output tapped from the middle of a register chain fences that
# chain; an independent cone still retimes.  The result is proven sequentially
# equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_port_tap.v
link_design top
create_clock -period 0.15 clk

write_verilog_for_dsec retime_dsec_port_tap before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
run_dsec_test retime_dsec_port_tap -liberty_files Nangate45/Nangate45_typ.lib \
  -clock clk -resetn rst_n

set v_file [make_result_file retime_dsec_port_tap.v]
write_verilog $v_file
diff_files retime_dsec_port_tap.vok $v_file
