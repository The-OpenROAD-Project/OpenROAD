# init_floorplan -core_area non-multiples of site size
source "helpers.tcl"
read_lef liberty1.lef
read_def reg1.def
read_liberty liberty1.lib
initialize_floorplan -die_area "0 0 1000 1000" \
  -core_area "110 110 900 900" \
  -site site1
auto_place_pins M1

set def_file [make_result_file init_floorplan4.def]
write_def $def_file
diff_files $def_file init_floorplan4.defok
