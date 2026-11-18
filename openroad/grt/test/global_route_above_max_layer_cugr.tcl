# A pin above the max routing layer must not abort CUGR global routing.
# Keep 3 routable layers: a met2 ceiling hits the separate two-layer gap.
source "helpers.tcl"
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

set_routing_layers -signal met1-met3
global_route -use_cugr

set guide_file [make_result_file global_route_above_max_layer_cugr.guide]
write_guides $guide_file
diff_files global_route_above_max_layer_cugr.guideok $guide_file
