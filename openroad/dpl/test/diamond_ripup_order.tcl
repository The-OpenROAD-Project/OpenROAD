# The diamond legalizer rips up and re-places the cells around a cell whose
# diamond search failed. The re-placement order must not depend on where the
# cells were allocated, so legalizing the same placement twice in one
# process, with the cells allocated at different addresses the second time,
# has to give the same result.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def aes_cipher_top_replace.def

set block [ord::get_db_block]
set initial {}
foreach inst [$block getInsts] {
  lappend initial [list $inst [$inst getLocation] [$inst getOrient] \
    [$inst getPlacementStatus]]
}

proc restore_placement { initial } {
  foreach entry $initial {
    lassign $entry inst loc orient status
    $inst setOrient $orient
    $inst setLocation {*}$loc
    $inst setPlacementStatus $status
  }
}

proc placement_snapshot { block } {
  set result {}
  foreach inst [$block getInsts] {
    lappend result "[$inst getName] [$inst getLocation] [$inst getOrient]"
  }
  return $result
}

# A small search window makes diamond searches fail, so rip-up runs.
catch { detailed_placement -use_diamond_legalizer -max_displacement {2 2} }
set first [placement_snapshot $block]

restore_placement $initial
catch { detailed_placement -use_diamond_legalizer -max_displacement {2 2} }
set second [placement_snapshot $block]

if { $first eq $second } {
  puts "placements match"
} else {
  puts "placements differ"
}
