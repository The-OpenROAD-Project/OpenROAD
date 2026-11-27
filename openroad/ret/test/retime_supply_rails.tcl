# Supply pins connected by hand, on more than one power net and with no
# global-connect rule.  A new cell takes the supply nets of the register it
# replaces.  A pin with no pin of its name there takes the net of its kind:
# rp has VDD and VSS, its replacement, the base cell DFFP_X1, has VPWR, VPB
# and VGND.  A pin with a namesake is connected like it, open included: rq
# has VPB open.  Registers on different supply nets are not merged: ra is on
# VDD_A and rb on VDD_B.  Both move up to gm in the same trial, each
# replacement on its own power net, and gm is not crossed.  rs stays: its
# power pins are on two nets, and the gates an unmapping makes have one power
# pin of another name.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty retime_supply_rails.lib
read_lef Nangate45/Nangate45.lef
read_lef retime_supply_rails.lef
read_verilog retime_supply_rails.v
link_design top
create_clock -period 0.13 clk

set block [ord::get_db_block]
foreach {name type} {VDD_A POWER VDD_B POWER VBIAS POWER VSS GROUND} {
  set net [odb::dbNet_create $block $name]
  $net setSigType $type
  $net setSpecial
}
proc connect_supply { inst pin net } {
  set iterm [$inst findITerm $pin]
  $iterm connect [[ord::get_db_block] findNet $net]
  $iterm setSpecial
}
foreach inst [$block getInsts] {
  foreach iterm [$inst getITerms] {
    set type [$iterm getSigType]
    if { $type == "POWER" || $type == "GROUND" } {
      set net [expr { $type == "POWER" ? "VDD_A" : "VSS" }]
      connect_supply $inst [[$iterm getMTerm] getName] $net
    }
  }
}
connect_supply [$block findInst rb] VDD VDD_B
connect_supply [$block findInst rs] VPB VBIAS
[[$block findInst rq] findITerm VPB] disconnect

retime
set lines {}
foreach inst [$block getInsts] {
  if { ![[$inst getMaster] isSequential] && ![string match "unmap_*" [$inst getName]] } {
    continue
  }
  set pins {}
  foreach iterm [$inst getITerms] {
    if { [$iterm getSigType] == "POWER" || [$iterm getSigType] == "GROUND" } {
      set net [$iterm getNet]
      set net_name [expr { $net == "NULL" ? "open" : [$net getName] }]
      lappend pins "[[$iterm getMTerm] getName]=$net_name"
    }
  }
  lappend lines "[$inst getName] ([[$inst getMaster] getName]): [join [lsort $pins] { }]"
}
puts [join [lsort $lines] "\n"]
