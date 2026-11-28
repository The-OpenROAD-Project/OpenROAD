# Marks in the database keep registers and gates in place.  The register r1
# stays when it is marked don't-touch, fixed in place, in a group or in a
# region, or when the net it drives has a non-default routing rule: the gate
# h1 that reads it is pinned then (3 pinned gates, not 2).  A don't-touch h1
# fences r1 (1 fenced register), and h1 in a group is pinned.  A don't-touch
# n2 or g2, or the rule on n2, pins g2 and h1, which drives it (4 pinned
# gates); h1 comes after g2 in the block, so this holds whatever the order
# of the gates.  A pin of the cell of r1 marked as a scan pin, or r1 in a
# scan chain of the database as a DEF SCANCHAINS section puts it, leaves r1
# out of the graph (1 register, not 2; none for the mark, as r2 has the same
# cell).  The rule on the clock net holds nothing: that retime moves r1, and
# the clock net is still the only net with the rule.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dont_touch.v
link_design top
create_clock -period 0.1 clk
set order {}
foreach inst [[ord::get_db_block] getInsts] {
  lappend order [$inst getName]
}
puts "block order: $order"

proc retime_case { name } {
  retime
  set r1 [[ord::get_db_block] findInst r1]
  puts "$name: r1 [expr { $r1 == "NULL" ? "moved" : "stays" }]"
}

proc nets_with_rule { } {
  set names {}
  foreach net [[ord::get_db_block] getNets] {
    if { [$net getNonDefaultRule] != "NULL" } {
      lappend names [$net getName]
    }
  }
  return [lsort $names]
}

foreach name {r1 n2 h1 g2} {
  set_dont_touch $name
  retime_case "don't-touch $name"
  unset_dont_touch $name
}

set block [ord::get_db_block]
set r1 [$block findInst r1]
$r1 setPlacementStatus FIRM
retime_case "r1 fixed"
$r1 setPlacementStatus NONE

set group [odb::dbGroup_create $block domain]
$group addInst $r1
retime_case "r1 in a group"
$group removeInst $r1

set region [odb::dbRegion_create $block area]
$region addInst $r1
retime_case "r1 in a region"
$region removeInst $r1

set h1 [$block findInst h1]
$group addInst $h1
retime_case "h1 in a group"
$group removeInst $h1

set scan_pin [[[ord::get_db] findMaster DFF_X1] findMTerm QN]
set sig_type [$scan_pin getSigType]
$scan_pin setSigType SCAN
retime_case "scan pin on DFF_X1"
$scan_pin setSigType $sig_type

set dft [$block getDft]
set chain [odb::dbScanChain_create $dft]
set scan_list [odb::dbScanList_create [odb::dbScanPartition_create $chain]]
$scan_list add $r1
retime_case "r1 in a scan chain"
$dft reset

set rule [odb::dbTechNonDefaultRule_create $block wide]
foreach name {q1 n2 clk} {
  [$block findNet $name] setNonDefaultRule $rule
  retime_case "rule on $name"
  puts "nets with the rule: [nets_with_rule]"
  [$block findNet $name] setNonDefaultRule NULL
}
