# pad_pin_edge with CUGR: a journal undo restores the pad nets from guides.
source "helpers.tcl"
read_lef "gf180/gf180mcu_6LM_1TM_9K_7t_tech.lef"
read_lef "gf180/gf180mcu_fd_sc_mcu7t5v0.lef"
read_lef "pad_pin_edge.lef"
read_def "pad_pin_edge.def"
make_tracks

set_routing_layers -signal Metal2-Metal5
global_route -use_cugr

# Reroute every pad net inside an ECO.
set block [ord::get_db_block]
odb::dbDatabase_beginEco $block
global_route -start_incremental
foreach name { buf buf2 buf3 buf4 } {
  set inst [$block findInst $name]
  lassign [$inst getLocation] x y
  $inst setLocation $x [expr { $y + 100000 }]
}
global_route -end_incremental

# The undo restores guides that reach each bond-pad port only via its pad pin.
global_route -start_incremental
odb::dbDatabase_undoEco $block
global_route -end_incremental

set guide_file [make_result_file pad_pin_edge_restore_cugr.guide]
write_guides $guide_file
diff_files pad_pin_edge_restore_cugr.guideok $guide_file

estimate_parasitics -global_routing
