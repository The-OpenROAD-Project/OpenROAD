# -place_ios with a 2D top-layer (up:) constraint region. The constrained pins
# move in 2D inside the box on the pin shape pattern layer; the rest stay on
# the die perimeter.

source helpers.tcl
set test_name place_ios_top_layer
read_lef ./nangate45.lef
read_def ./simple01.def

# simple01 ships its ports FIXED, which -place_ios keeps as anchors.
set block [ord::get_db_block]
foreach bterm [$block getBTerms] {
  foreach bpin [$bterm getBPins] {
    $bpin setPlacementStatus PLACED
  }
}

define_pin_shape_pattern -layer metal10 -x_step 1.6 -y_step 1.6 \
  -region { 5 5 25 25 } -size { 1.6 2.5 }
set constrained {clk reset req_val resp_rdy}
set_io_pin_constraint -pin_names $constrained -region "up:{10 10 20 20}"

global_placement -place_ios -init_density_penalty 0.01 -skip_initial_place

# up:{10 10 20 20} in a 2000 DBU/um design.
set region_lo 20000
set region_hi 40000
set die [$block getDieArea]

foreach bterm [$block getBTerms] {
  set name [$bterm getName]
  set box [lindex [[lindex [$bterm getBPins] 0] getBoxes] 0]
  set layer [[$box getTechLayer] getName]
  set cx [expr { ([$box xMin] + [$box xMax]) / 2 }]
  set cy [expr { ([$box yMin] + [$box yMax]) / 2 }]

  if { [lsearch -exact $constrained $name] >= 0 } {
    if { $layer ne "metal10" } {
      error "$name is on $layer, expected the metal10 pin shape pattern layer"
    }
    if {
      $cx < $region_lo || $cx > $region_hi
      || $cy < $region_lo || $cy > $region_hi
    } {
      error "$name is at ($cx $cy), outside its up: region"
    }
  } else {
    if {
      $cx != [$die xMin] && $cx != [$die xMax]
      && $cy != [$die yMin] && $cy != [$die yMax]
    } {
      error "$name is at ($cx $cy), off the die perimeter"
    }
  }
}

puts pass
