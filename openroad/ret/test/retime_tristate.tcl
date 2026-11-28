# A cell that is not a plain combinational gate is never crossed, and a
# register with a tri-state output stays, on failing paths too.  The
# registers in front of the tri-state driver t1 stay, and so does the one in
# front of the delay cell u1, whose output has no liberty function.  The
# tri-state register rt stays: the register that would replace it always
# drives.  t1, u1 and j1 behind rt are pinned (10 pinned gates, not 7).
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty retime_tristate.lib
read_lef Nangate45/Nangate45.lef
read_lef retime_tristate.lef
read_verilog retime_tristate.v
link_design top
create_clock -period 0.13 clk

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
set block [ord::get_db_block]
foreach name {r1 re rt ru} {
  set state [expr { [$block findInst $name] == "NULL" ? "removed" : "stays" }]
  puts "$name: $state"
}
