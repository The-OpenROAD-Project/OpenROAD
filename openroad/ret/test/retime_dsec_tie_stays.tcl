# retime_dsec_async_tied removes the tie cells t1 and t2 with the registers
# they held.  Here both are protected: t1 is fixed in place and the net of
# t2 has a constraint.  The registers move as before and leave both tie
# cells in place.  The result is proven sequentially equivalent with ABC
# dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_async_tied.v
link_design top
create_clock -period 0.15 clk
set block [ord::get_db_block]
[$block findInst t1] setPlacementStatus FIRM
set_timing_derate -net_delay -late 1.0 [get_nets hi2]

write_verilog_for_dsec retime_dsec_tie_stays before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
foreach name {r1 r2 t1 t2} {
  puts "$name: [expr { [$block findInst $name] == "NULL" ? "removed" : "stays" }]"
}
run_dsec_test retime_dsec_tie_stays \
  -liberty_files Nangate45/Nangate45_typ.lib -clock clk -resetn rst_n
