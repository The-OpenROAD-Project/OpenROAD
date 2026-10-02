# -place_ios with -timing_driven. repair_design creates and destroys
# instances mid-solve, which permutes gCellStor_ and reallocates gPinStor_,
# while the IO pin GCells live in their own storage: every pin must still own
# its GPin afterwards and end up on the die perimeter.

source helpers.tcl
read_liberty ./library/nangate45/NangateOpenCellLibrary_typical.lib

read_lef ./nangate45.lef
read_def ./simple01-td-repair.def

create_clock -name core_clock -period 0.5 clk

set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5

# The def places the ports FIXED, which -place_ios keeps as anchors.
set block [ord::get_db_block]
foreach bterm [$block getBTerms] {
  foreach bpin [$bterm getBPins] {
    $bpin setPlacementStatus PLACED
  }
}
set insts_before [llength [$block getInsts]]

# How the pins are to be placed: the solve reads it to model the slot grid
# place_pins will assign on, so rsz and STA see shapes it can reproduce.
set_place_config -io_pin_hor_layers metal5 -io_pin_ver_layers metal6
global_placement -timing_driven -timing_driven_repair_timing -place_ios

puts "instance count: $insts_before -> [llength [$block getInsts]]"

# A pin that lost its GPin in the churn would stop moving, so check that every
# one of them ends on the perimeter.
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
  # The solve assigns the pins to tracks itself, so each one sits on a track of
  # its own layer rather than exactly on the die edge.
  set layer [$box getTechLayer]
  set grid [$block findTrackGrid $layer]
  if { [$layer getDirection] eq "HORIZONTAL" } {
    set coord $cy
    set tracks [$grid getGridY]
  } else {
    set coord $cx
    set tracks [$grid getGridX]
  }
  if { [lsearch -exact -integer $tracks $coord] < 0 } {
    error "$name is at $coord on [$layer getName], not on a track"
  }
}

estimate_parasitics -placement
report_worst_slack

puts pass
