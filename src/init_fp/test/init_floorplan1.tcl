# init_floorplan
source "../../../test/helpers.tcl"
read_lef liberty1.lef
read_def reg1.def
read_liberty liberty1.lib
initialize_floorplan -die_area "0 0 1000 1000" \
  -core_area "100 100 900 900" \
  -site site1
auto_place_pins M1

set def_file [make_result_file init_floorplan1.def]
write_def $def_file
diff_files init_floorplan1.defok $def_file
