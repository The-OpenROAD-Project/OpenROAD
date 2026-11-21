# Inputs arrive late: the register chain would balance by sliding r1 forward, but
# every slide lengthens the input path a/b -> g0 -> ... past the period, so retime
# must reject the candidates and change nothing.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 1.0 clk
set_input_delay -clock clk 0.97 [get_ports {a b}]

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4

set v_file [make_result_file retime_input_limit.v]
write_verilog $v_file
diff_files retime_input_limit.vok $v_file
