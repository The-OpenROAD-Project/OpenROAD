# A late input limits retiming only at the register it feeds.  When a
# arrives late, the path from a into r1 fails and every slide of r1 would
# lengthen it, so none is proposed and r1 stays.  With the input delay on c
# instead, c -> t is the worst path, into r2, and r1 slides: the slides
# across i1, j1 and j2 cannot raise r2's slack, which stays at the slack of
# the path from c, but they raise r3's, so the search takes all three and
# the TNS improves.  The result is proven sequentially equivalent with ABC
# dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_input_path.v
link_design top
create_clock -period 0.1 clk
write_verilog_for_dsec retime_input_path before

set_input_delay 0.97 -clock clk [get_ports a]
retime
set r1 [[ord::get_db_block] findInst r1]
puts "r1: [expr { $r1 == "NULL" ? "moved" : "stays" }]"
unset_input_delay -clock clk [get_ports a]

set_input_delay 0.15 -clock clk [get_ports c]
report_worst_slack -max -digits 4
report_tns -digits 4
retime
report_worst_slack -max -digits 4
report_tns -digits 4
run_dsec_test retime_input_path \
  -liberty_files Nangate45/Nangate45_typ.lib -clock clk -resetn rst_n
