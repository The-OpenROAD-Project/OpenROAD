# A trial that creates a hold violation is not kept.  retime_basic with a
# hold uncertainty: its three slides leave too little delay behind the new
# register, so STA rejects them and keeps the one slide that still meets
# hold.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 0.13 clk
set_clock_uncertainty -hold 0.10 clk

report_worst_slack -max -digits 4
report_worst_slack -min -digits 4
retime
report_worst_slack -max -digits 4
report_worst_slack -min -digits 4

set v_file [make_result_file retime_hold.v]
write_verilog $v_file
diff_files retime_hold.vok $v_file
