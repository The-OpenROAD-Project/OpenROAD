# Colocated M1/M3 pins retain both layers when CUGR folds maze-tree nodes.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "cugr_colocated_pins.def"
set_routing_layers -signal metal2-metal3
# The sole vertical layer stays congested, forcing this net into maze routing.
set_global_routing_layer_adjustment metal2 0.99

set block [ord::get_db_block]
set segments [make_result_file cugr_colocated_pins.segs]
foreach mode {shapes odb mixed} {
  foreach bterm [$block getBTerms] {
    foreach bpin [$bterm getBPins] {
      foreach ap [$bpin getAccessPoints] {
        odb::dbAccessPoint_destroy $ap
      }
      set box [lindex [$bpin getBoxes] 0]
      set layer [$box getTechLayer]
      if { $mode == "odb" || ($mode == "mixed" && [$layer getName] == "metal1") } {
        set x [expr { ([$box xMin] + [$box xMax]) / 2 }]
        set y [expr { ([$box yMin] + [$box yMax]) / 2 }]
        set ap [odb::dbAccessPoint_create $bpin]
        $ap setPoint [odb::Point new $x $y]
        $ap setLayer $layer
      }
    }
  }
  with_output_to_variable route_log {
    global_route -use_cugr -verbose -allow_congestion -congestion_iterations 1
  }
  check "$mode reaches maze routing" {
    string match "*Stage 4: Maze routing on sparsified graph.*" $route_log
  } 1
  write_global_route_segments $segments
  check "$mode preserves M1-M3 connections at both endpoints" {
    diff_files cugr_colocated_pins.segsok $segments
  } 0
}
exit_summary
