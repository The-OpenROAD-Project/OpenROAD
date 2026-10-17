# Post-DRT repair_antennas on a congested CUGR route: nets the overflow
# relief moves must drop their detailed wires, like the dirty nets.
source "helpers.tcl"
# Suppress DRT init logging: region/guide query sizes are a function of the
# exact route geometry, not the repair contract this test locks.
suppress_message DRT 33
suppress_message DRT 36
suppress_message DRT 167
suppress_message DRT 168
suppress_message DRT 178
suppress_message DRT 179
suppress_message DRT 349
read_liberty "sky130hs/sky130hs_tt.lib"
read_lef "sky130hs/sky130hs.tlef"
read_lef "sky130hs/sky130hs_std_cell.lef"
read_def "gcd_sky130.def"

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met1-met3 0.8
set_routing_layers -signal met1-met3
global_route -use_cugr

detailed_route -verbose 0

proc guide_geometry { net } {
  set rows {}
  foreach guide [$net getGuides] {
    set box [$guide getBox]
    lappend rows [list [$box xMin] [$box yMin] [$box xMax] [$box yMax] \
      [[$guide getLayer] getName]]
  }
  return [lsort $rows]
}

# Record each routed net's wire and guides before the repair.
set block [ord::get_db_block]
foreach net [$block getNets] {
  if { [$net getWire] ne "NULL" } {
    set prior_wire([$net getName]) [$net getWire]
    set prior_guides([$net getName]) [guide_geometry $net]
  }
}

set_debug_level GRT verify_demand 1
repair_antennas

# A routed net with new guides must have dropped its old wire.
set rerouted 0
set stale 0
foreach net [$block getNets] {
  set name [$net getName]
  if {
    [info exists prior_wire($name)]
    && [guide_geometry $net] ne $prior_guides($name)
  } {
    incr rerouted
    if { [$net getWire] eq $prior_wire($name) } {
      puts "Net $name keeps its old wire"
      incr stale
    }
  }
}
puts "Routed nets with new guides: $rerouted, keeping their old wire: $stale"
