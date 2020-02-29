# init_floorplan -tracks non-zero diearea origin
source "helpers.tcl"
read_lef liberty1.lef
read_def reg1.def
read_liberty liberty1.lib
initialize_floorplan -die_area "10 20 1010 1020" \
  -core_area "110 120 910 920" \
  -site site1 \
  -tracks init_floorplan2.tracks
auto_place_pins M1

set def_file [make_result_file init_floorplan7.def]
write_def $def_file
diff_files $def_file init_floorplan7.defok
