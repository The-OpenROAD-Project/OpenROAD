# Post-detailed-route repair_antennas in a CUGR session when a pin sits above
# the routing ceiling: its demand must be adopted, not counted as a failure.
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

# Move one bterm from met3 up to met4, leaving its only shape above the ceiling.
set block [ord::get_db_block]
set met4 [[ord::get_db_tech] findLayer met4]
foreach bterm [$block getBTerms] {
  set bpin [lindex [$bterm getBPins] 0]
  set box [lindex [$bpin getBoxes] 0]
  if { [[$box getTechLayer] getName] eq "met3" } {
    set new_bpin [odb::dbBPin_create $bterm]
    odb::dbBox_create $new_bpin $met4 [$box xMin] [$box yMin] [$box xMax] \
      [$box yMax]
    $new_bpin setPlacementStatus "PLACED"
    odb::dbBPin_destroy $bpin
    puts "moved bterm [$bterm getName] from met3 to met4"
    break
  }
}

set_placement_padding -global -left 2 -right 2
set_global_routing_layer_adjustment met2-met3 0.15
set_routing_layers -signal met1-met3
global_route -use_cugr

detailed_route -verbose 0

set_debug_level GRT repair_antennas 1
check_antennas
repair_antennas
check_antennas
check_placement
