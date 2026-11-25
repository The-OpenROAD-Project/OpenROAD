# An input-to-register path longer than any register-to-register path must not
# block retiming: the r1 -> r2 chain still gets its slide.
# top_pinned: the input delay on c makes c -> t the worst path into r2.  The
# slides across i1, j1 and j2 cannot raise r2's slack, which stays at the
# slack of the path from c, but they raise r3's: the search takes all three
# and the TNS improves.  The result is proven sequentially equivalent with
# ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
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

read_verilog retime_input_path.v
link_design top_pinned
create_clock -period 0.1 clk
set_input_delay 0.15 -clock clk [get_ports c]

write_verilog_for_dsec retime_input_path_pinned before
report_worst_slack -max -digits 4
report_tns -digits 4
retime
report_worst_slack -max -digits 4
report_tns -digits 4
run_dsec_test retime_input_path_pinned \
  -liberty_files Nangate45/Nangate45_typ.lib -clock clk -resetn rst_n
