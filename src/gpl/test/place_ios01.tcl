# -place_ios on ports with no placement at all: the shapes are removed from
# every bterm, so the initial B2B solve has to skip them and Nesterov has to
# place every one of them on the die perimeter. Also covers the two hard
# constraints the solve has to honor -- mirrored pairs and excluded regions --
# and ends by excluding the rest of the perimeter, which must be rejected
# instead of silently placing pins into an excluded region.

source helpers.tcl
set test_name place_ios01
read_lef ./nangate45.lef
read_def ./simple01.def

set block [ord::get_db_block]
foreach bterm [$block getBTerms] {
  foreach bpin [$bterm getBPins] {
    odb::dbBPin_destroy $bpin
  }
  if { ![[$bterm getBBox] isInverted] } {
    error "[$bterm getName] still has a shape after destroying its bpins"
  }
}

set mirror_pairs {req_msg[0] resp_msg[0] req_msg[1] resp_msg[1]}
set_io_pin_constraint -mirrored_pins $mirror_pairs
exclude_io_pin_region -region top:*

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
  set name [$bterm getName]
  lassign [pin_center $bterm] cx cy

  if {
    $cx != [$die xMin] && $cx != [$die xMax]
    && $cy != [$die yMin] && $cy != [$die yMax]
  } {
    error "$name is at ($cx $cy), off the die perimeter"
  }
  # The whole top edge was excluded before the solve.
  if { $cy == [$die yMax] } {
    error "$name is at ($cx $cy), on the excluded top edge"
  }
}

# A mirrored pair must end up as an exact reflection across the die, on the
# axis of the master's own edge.
foreach { master follower } $mirror_pairs {
  lassign [pin_center [$block findBTerm $master]] mx my
  lassign [pin_center [$block findBTerm $follower]] fx fy

  if { $mx == [$die xMin] || $mx == [$die xMax] } {
    set want_x [expr { $mx == [$die xMin] ? [$die xMax] : [$die xMin] }]
    if { $fx != $want_x || $fy != $my } {
      error "$follower is at ($fx $fy), not the reflection of\
             $master at ($mx $my)"
    }
  } else {
    set want_y [expr { $my == [$die yMin] ? [$die yMax] : [$die yMin] }]
    if { $fy != $want_y || $fx != $mx } {
      error "$follower is at ($fx $fy), not the reflection of\
             $master at ($mx $my)"
    }
  }
}

# With every edge excluded no perimeter pin has a legal position left.
exclude_io_pin_region -region bottom:* -region left:* -region right:*
if { [catch { global_placement -place_ios -init_density_penalty 0.01 } msg] } {
  if { ![string match "*GPL-0180*" $msg] } {
    error "Expected GPL-0180 with the whole perimeter excluded, got: $msg"
  }
} else {
  error "Expected -place_ios to be rejected with the whole perimeter excluded"
}

puts pass
