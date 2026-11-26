# retime_dsec_async_tied with the supply pin of the tie cell t1 on a
# don't-touch net.  Deleting t1 would disconnect that net, so t1 stays when
# the register it held moves.  The result is proven sequentially equivalent
# with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_async_tied.v
link_design top
create_clock -period 0.15 clk

write_verilog_for_dsec retime_dsec_tie_supply before
set block [ord::get_db_block]
set supply [odb::dbNet_create $block VDD]
$supply setSigType POWER
[[$block findInst t1] findITerm VDD] connect $supply
$supply setDoNotTouch 1

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
foreach name {r1 t1 t2} {
  puts "$name: [expr { [$block findInst $name] == "NULL" ? "removed" : "stays" }]"
}
run_dsec_test retime_dsec_tie_supply \
  -liberty_files Nangate45/Nangate45_typ.lib -clock clk -resetn rst_n
