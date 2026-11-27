# A register with a tri-state output stays, on a failing path too: the
# register that would replace it always drives.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty retime_tristate_register.lib
read_lef Nangate45/Nangate45.lef
read_lef retime_tristate_register.lef
read_verilog retime_tristate_register.v
link_design top
create_clock -period 0.13 clk

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
set r1 [[ord::get_db_block] findInst r1]
puts "r1 is [[$r1 getMaster] getName], OE reads [[[$r1 findITerm OE] getNet] getName]"
