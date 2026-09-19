# Sixteen inverters whose only connections are to movable IO pins: nothing in
# the design is fixed, so the pins follow the cells that are pulled towards
# them and the solve never reaches the target overflow. It runs to a
# standstill instead -- the gradients stop changing, with nothing left to
# move. That is a placement that is over, and global placement has to finish
# it and leave a legal result, not report a diverged gradient descent.

source helpers.tcl
read_lef ./nangate45.lef
read_def ./place_ios03.def

set block [ord::get_db_block]

global_placement -place_ios

proc pin_center { bterm } {
  set bpins [$bterm getBPins]
  if { [llength $bpins] != 1 } {
    error "[$bterm getName] has [llength $bpins] bpins, expected exactly 1"
  }
  set bpin [lindex $bpins 0]
  if { [$bpin getPlacementStatus] ne "PLACED" } {
    error "[$bterm getName] is [$bpin getPlacementStatus], expected PLACED"
  }
  set box [lindex [$bpin getBoxes] 0]
  return [list [expr { ([$box xMin] + [$box xMax]) / 2 }] \
    [expr { ([$box yMin] + [$box yMax]) / 2 }]]
}

set die [$block getDieArea]
foreach bterm [$block getBTerms] {
  lassign [pin_center $bterm] cx cy
  if {
    $cx != [$die xMin] && $cx != [$die xMax]
    && $cy != [$die yMin] && $cy != [$die yMax]
  } {
    error "[$bterm getName] is at ($cx $cy), off the die perimeter"
  }
}

foreach inst [$block getInsts] {
  lassign [$inst getLocation] x y
  if { $x < [$die xMin] || $x > [$die xMax] || $y < [$die yMin] || $y > [$die yMax] } {
    error "[$inst getName] is at ($x $y), outside the die"
  }
}

puts pass
