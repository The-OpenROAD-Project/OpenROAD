# Forward retiming of a plain register chain on Nangate45.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 1.0 clk

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4

set v_file [make_result_file retime_basic.v]
write_verilog $v_file
diff_files retime_basic.vok $v_file
