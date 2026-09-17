# Post-detailed-route repair_antennas in a CUGR session when a detailed wire
# carries a pin-access span below the min routing layer that crosses gcell
# boundaries: demand adoption must treat it as inert instead of aborting.
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
set_global_routing_layer_adjustment met2-met3 0.15
set_routing_layers -signal met1-met3
global_route -use_cugr

detailed_route -verbose 0

# DRT rarely leaves such a span in this small design, so append a vertical
# li1 (below-min, preferred-direction) span to one routed net, anchored at a
# pin so it stays gcell-connected to the real route.
set block [ord::get_db_block]
set li1 [[ord::get_db_tech] findLayer li1]
set target ""
foreach net [$block getNets] {
  if { [$net isSpecial] || [$net getWire] == "NULL" } { continue }
  if { [llength [$net getITerms]] < 2 } { continue }
  set target $net
  break
}
set bbox [[lindex [$target getITerms] 0] getBBox]
set x [expr { ([$bbox xMin] + [$bbox xMax]) / 2 }]
set y [expr { ([$bbox yMin] + [$bbox yMax]) / 2 }]
set enc [odb::dbWireEncoder]
$enc append [$target getWire]
$enc newPath $li1 "ROUTED"
$enc addPoint $x $y
$enc addPoint $x [expr { $y + 20000 }]
$enc end
puts "added li1 span to [$target getName]"

check_antennas
repair_antennas
check_antennas
check_placement
