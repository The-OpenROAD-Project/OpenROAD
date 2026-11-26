# An asynchronous reset counts as tied inactive only when a tie cell is the
# one driver of its net.  With a second driver on the net the register has a
# live reset and never moves.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_reset_drivers.v
link_design top
create_clock -period 0.13 clk

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
set r1 [[ord::get_db_block] findInst r1]
puts "r1: [expr { $r1 == "NULL" ? "removed" : [[$r1 getMaster] getName] }]"
