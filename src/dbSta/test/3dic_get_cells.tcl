source "helpers.tcl"

# Hierarchical top dbChip must expose its dbChipInst children via
# dbNetwork::DbInstanceChildIterator, so `get_cells *` returns the chiplet
# instance names.
#
# example.bmap leaves both bumps unmapped (col-4 = "-"). v1 STA does not
# warn on this (it's a legitimate blackbox-stage state); paths through
# such bumps drop silently. This fixture exercises the structural
# iteration APIs (get_cells / get_pins / get_nets), not signal traversal.
read_3dbx ../../odb/test/data/example.3dbx

proc cell_names { } {
  set names {}
  foreach c [get_cells *] {
    lappend names [get_full_name $c]
  }
  return [lsort $names]
}

check "chiplet count" { llength [get_cells *] } 2
check "chiplet names" { cell_names } {soc_inst soc_inst_duplicate}

# Stage 4: chip-inst pin iteration must yield one Pin per bump-inst.
# example.bmap defines 2 bumps (bump1, bump2) per SoC chiplet master.
check "soc_inst pin count" { llength [get_pins -of_objects [get_cells soc_inst]] } 2
check "soc_inst_dup pin count" {
  llength [get_pins -of_objects [get_cells soc_inst_duplicate]]
} 2

# Stage 5: dbChipNet iteration. example.3dbx ships no chip-nets (its
# top.v reference is unresolved at parse time), so build one
# programmatically: tie soc_inst's first bump to soc_inst_duplicate's
# first bump as a synthetic cross-chiplet net.
set db [ord::get_db]
set top_chip [$db getChip]
set chip_inst_a [$top_chip findChipInst soc_inst]
set chip_inst_b [$top_chip findChipInst soc_inst_duplicate]
proc first_bump_inst { chip_inst } {
  foreach region [$chip_inst getRegions] {
    foreach bump [$region getChipBumpInsts] {
      return $bump
    }
  }
  return ""
}
set bump_a [first_bump_inst $chip_inst_a]
set bump_b [first_bump_inst $chip_inst_b]
set chip_net [odb::dbChipNet_create $top_chip "bridge"]
$chip_net addBumpInst $bump_a [list $chip_inst_a]
$chip_net addBumpInst $bump_b [list $chip_inst_b]

# DbInstanceNetIterator picks up the newly created chip-net.
check "chip-net count" { llength [get_nets *] } 1
# DbNetPinIterator yields one chip-bump-inst Pin per bump on the net.
check "bridge pin count" {
  llength [get_pins -of_objects [get_nets bridge]]
} 2
# Each pin's owning instance is a distinct chiplet — proves the net
# spans BOTH chiplets, not just lists two bumps on one chiplet.
proc bridge_pin_instances { } {
  set names {}
  foreach pin [get_pins -of_objects [get_nets bridge]] {
    lappend names [get_full_name [$pin instance]]
  }
  return [lsort $names]
}
check "bridge spans both chiplets" { bridge_pin_instances } \
  {soc_inst soc_inst_duplicate}

exit_summary
