# The supply pins of every cell are connected by hand and there is no
# global-connect rule (a DEF with explicit connections).  Each cell retime
# creates takes the supply nets of the cell it stands for: the new register
# and its tie cell (ihp-sg13g2 has no plain flip-flop), and the inverter left
# from unmapping r1, which is read through Q_N.
source "helpers.tcl"
read_liberty ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib
read_lef ihp-sg13g2/sg13g2_tech.lef
read_lef ihp-sg13g2/sg13g2_stdcell.lef
read_verilog retime_dsec_hier_tied_base.v
link_design top
create_clock -period 0.4 clk

set block [ord::get_db_block]
foreach {name type} {VDD POWER VSS GROUND} {
  set net [odb::dbNet_create $block $name]
  $net setSigType $type
  $net setSpecial
  foreach inst [$block getInsts] {
    foreach iterm [$inst getITerms] {
      if { [$iterm getSigType] == $type } {
        $iterm connect $net
        $iterm setSpecial
      }
    }
  }
}

retime
set lines {}
foreach inst [$block getInsts] {
  set pins {}
  foreach iterm [$inst getITerms] {
    if { [$iterm getSigType] == "POWER" || [$iterm getSigType] == "GROUND" } {
      set net [$iterm getNet]
      set net_name [expr { $net == "NULL" ? "open" : [$net getName] }]
      lappend pins "[[$iterm getMTerm] getName]=$net_name special=[$iterm isSpecial]"
    }
  }
  if { [string match "ret_*" [$inst getName]] || [string match "unmap_*" [$inst getName]] } {
    lappend lines "[$inst getName]: [join [lsort $pins] {, }]"
  }
}
puts [join [lsort $lines] "\n"]
