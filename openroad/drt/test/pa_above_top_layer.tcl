# Regression test for https://github.com/The-OpenROAD-Project/OpenROAD/issues/11812
#
# The X pin of sky130_fd_sc_hd__probec_p_8 has shapes on li1 through met5.
# With the top routing layer set to met4, pin access used to keep access
# points on met5. The detailed-route grid graph stops at the top routing
# layer, so those access points had no maze index and detailed_route
# crashed in FlexDRWorker::routeNet after "initMazeIdx_ap no idx".
source "helpers.tcl"
read_lef "sky130hd/sky130hd.tlef"
read_lef "sky130hd/sky130_fd_sc_hd_merged.lef"
read_liberty "sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib"
read_verilog "pa_above_top_layer.v"
link_design top

initialize_floorplan -die_area {0 0 40 40} \
  -core_area {5 5 35 35} \
  -site unithd
make_tracks
place_pins -hor_layers met3 -ver_layers met2

# Row 5, site 22: a legal location, so no detailed placement is needed
set inst [[ord::get_db_block] findInst u1]
$inst setLocation [ord::microns_to_dbu 15.18] [ord::microns_to_dbu 19.04]
$inst setPlacementStatus PLACED

set_routing_layers -signal met1-met4
global_route
detailed_route -verbose 0

set def_file [make_result_file pa_above_top_layer.def]
write_def $def_file
diff_files pa_above_top_layer.defok $def_file
