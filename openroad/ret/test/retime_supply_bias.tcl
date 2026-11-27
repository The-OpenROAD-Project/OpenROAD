# A netlist with its supply pins wired, in a library whose cells all have a
# well pin (VPB) next to the power pin (VPWR): the wells are on a bias net.
# The register's power pins are on two nets, and it still moves, because
# every cell retime creates has those pins: the new register is connected
# pin by pin like the one it replaces.  read_verilog does not type the three
# nets as supplies, which changes nothing.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef retime_supply_bias.lef
read_liberty retime_supply_bias.lib
read_verilog retime_supply_bias.v
link_design top
create_clock -period 0.16 clk

retime
set lines {}
foreach inst [[ord::get_db_block] getInsts] {
  if { ![[$inst getMaster] isSequential] } {
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
