# init_floorplan -utilization -aspect_ratio -core_space
source "helpers.tcl"
read_lef liberty1.lef
read_def reg1.def
read_liberty liberty1.lib
initialize_floorplan -utilization 30 \
  -aspect_ratio 0.5 \
  -core_space 2 \
  -tracks init_floorplan2.tracks \
  -site site1
auto_place_pins M1

set def_file [make_result_file init_floorplan3.def]
write_def $def_file
diff_files $def_file init_floorplan3.defok
