# A few movable cells against many movable pins. The pins cross the die while
# the cells move a fraction of a site, so the two displacements are orders of
# magnitude apart. The step length is a norm over the cells -- the pins are
# left out of it because they only slide along the perimeter -- and it has to
# stay one: global placement must finish and leave every pin on the die
# perimeter and every movable cell inside the die.

source helpers.tcl
read_lef ./nangate45.lef
read_def ./simple01.def

set block [ord::get_db_block]

# Every port movable: drop the shapes the DEF placed them with.
foreach bterm [$block getBTerms] {
  foreach bpin [$bterm getBPins] {
    odb::dbBPin_destroy $bpin
  }
}

# Four movable instances against 54 movable pins; the rest keep their DEF
# positions and anchor nothing else.
set movable {}
foreach inst [$block getInsts] {
  if { [llength $movable] < 4 } {
    lappend movable $inst
  } else {
    $inst setPlacementStatus FIRM
  }
}

global_placement -place_ios -init_density_penalty 0.01

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

foreach inst $movable {
  lassign [$inst getLocation] x y
  if { $x < [$die xMin] || $x > [$die xMax] || $y < [$die yMin] || $y > [$die yMax] } {
    error "[$inst getName] is at ($x $y), outside the die"
  }
}

puts pass
