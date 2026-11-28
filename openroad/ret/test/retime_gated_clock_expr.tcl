# A register clocked by an expression of several pins ("CK & EN") never
# moves, so the inverter it drives is never crossed; the same cone with a
# DFF_X1 in its place retimes.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty retime_gated_clock_expr.lib
read_lef Nangate45/Nangate45.lef
read_lef retime_gated_clock_expr.lef
read_verilog retime_gated_clock_expr.v
link_design top
create_clock -period 0.1 clk

retime
set block [ord::get_db_block]
foreach name {r1 rg} {
  set inst [$block findInst $name]
  if { $inst == "NULL" } {
    puts "$name: moved"
  } else {
    puts "$name: stays, a [[$inst getMaster] getName]"
  }
}
foreach name {g1 g8} {
  set net [[[$block findInst $name] findITerm A] getNet]
  puts "$name reads [$net getName]"
}
