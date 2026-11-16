# A don't-touch register that shares its net with a movable one pins the
# gate after the movable register; an independent cone still retimes.  The
# result is proven sequentially equivalent with ABC dsec when
# EQUIVALENCE_CHECK is set.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_dont_touch_group.v
link_design top
create_clock -period 1.0 clk
set_dont_touch rD

write_verilog_for_dsec retime_dsec_dont_touch_group before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
run_dsec_test retime_dsec_dont_touch_group \
  -liberty_files Nangate45/Nangate45_typ.lib -clock clk -resetn rst_n

set v_file [make_result_file retime_dsec_dont_touch_group.v]
write_verilog $v_file
diff_files retime_dsec_dont_touch_group.vok $v_file
