# A register that drives only an output port keeps its depth, so the gate
# in front of it is never crossed; the gates before that one still are.
# The result is proven sequentially equivalent with ABC dsec when
# EQUIVALENCE_CHECK is set.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_port_only.v
link_design top
create_clock -period 1.0 clk

write_verilog_for_dsec retime_dsec_port_only before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
run_dsec_test retime_dsec_port_only -liberty_files Nangate45/Nangate45_typ.lib \
  -clock clk -resetn rst_n

set v_file [make_result_file retime_dsec_port_only.v]
write_verilog $v_file
diff_files retime_dsec_port_only.vok $v_file
