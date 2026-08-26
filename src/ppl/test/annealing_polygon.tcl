# the annealing placer must handle polygon dies, where a pin orientation can
# not match the direction of the layer set it came from
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def gcd_polygon_pre_ppl-tcl.def

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
  -min_distance 0.12 -annealing

set placed 0
foreach bterm [[ord::get_db_block] getBTerms] {
  if { [$bterm getFirstPinPlacementStatus] == "PLACED" } {
    incr placed
  }
}
puts "placed pins: $placed"
