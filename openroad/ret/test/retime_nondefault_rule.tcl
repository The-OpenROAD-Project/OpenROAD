# A net with a non-default routing rule keeps its driver and its loads: a
# net made for them would not have the rule.  With the rule on q1, the net
# the register of retime_basic drives, the register stays; with it on n2,
# gate g2 is not crossed (4 pinned gates, not 2).  The clock net often has
# such a rule and holds nothing: the last retime moves the register.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 0.1 clk

set block [ord::get_db_block]
set rule [odb::dbTechNonDefaultRule_create $block wide]

proc nets_with_rule { } {
  set names {}
  foreach net [[ord::get_db_block] getNets] {
    if { [$net getNonDefaultRule] != "NULL" } {
      lappend names [$net getName]
    }
  }
  return [lsort $names]
}

foreach name {q1 n2 clk} {
  [$block findNet $name] setNonDefaultRule $rule
  retime
  puts "nets with the rule: [nets_with_rule]"
  [$block findNet $name] setNonDefaultRule NULL
}
