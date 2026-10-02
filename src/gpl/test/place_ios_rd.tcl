# -place_ios with -routability_driven. The routability loop inflates cells,
# rebuilds the congestion map from the database and reverts the whole solve to
# a snapshot; the IO pins take part in all three, so they must end on the die
# perimeter with the positions the router was shown.

source helpers.tcl
read_liberty ./library/nangate45/NangateOpenCellLibrary_typical.lib

read_lef ./nangate45.lef
read_def ./simple04-rd.def

# The def places the ports FIXED, which -place_ios keeps as anchors.
set block [ord::get_db_block]
foreach bterm [$block getBTerms] {
  foreach bpin [$bterm getBPins] {
    $bpin setPlacementStatus PLACED
  }
}

# A port on a reset net owns no GPin, but grt routes its net and rejects it if
# it stays unplaced, so the solve must still give it a slot. Leave one with no
# shape at all.
set unmodeled [$block findBTerm {req_msg[0]}]
[$unmodeled getNet] setSigType RESET
foreach bpin [$unmodeled getBPins] {
  odb::dbBPin_destroy $bpin
}

set_place_config -io_pin_hor_layers metal1 -io_pin_ver_layers metal2

# The target is low enough that the loop keeps inflating and reverting.
global_placement -routability_driven -routability_use_grt \
  -routability_target_rc_metric 0.20 -place_ios

set die [$block getDieArea]
foreach bterm [$block getBTerms] {
  set name [$bterm getName]
  set bpins [$bterm getBPins]
  if { [llength $bpins] != 1 } {
    error "$name has [llength $bpins] bpins, expected exactly 1"
  }
  set bpin [lindex $bpins 0]
  if { [$bpin getPlacementStatus] ne "PLACED" } {
    error "$name is [$bpin getPlacementStatus], expected PLACED"
  }
  set box [lindex [$bpin getBoxes] 0]
  set cx [expr { ([$box xMin] + [$box xMax]) / 2 }]
  set cy [expr { ([$box yMin] + [$box yMax]) / 2 }]
  # The pin box runs inward from the edge it sits on.
  if {
    [$box xMin] > [$die xMin] && [$box xMax] < [$die xMax]
    && [$box yMin] > [$die yMin] && [$box yMax] < [$die yMax]
  } {
    error "$name is at ($cx $cy), off the die perimeter"
  }
}

puts pass
