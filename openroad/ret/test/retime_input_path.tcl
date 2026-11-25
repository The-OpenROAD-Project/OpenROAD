# An input-to-register path longer than any register-to-register path must not
# block retiming: the r1 -> r2 chain still gets its slide.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_input_path.v
link_design top
create_clock -period 0.1 clk

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4

set v_file [make_result_file retime_input_path.v]
write_verilog $v_file
diff_files retime_input_path.vok $v_file
