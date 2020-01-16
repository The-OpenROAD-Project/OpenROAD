# init_floorplan UNITS 2000
source "helpers.tcl"
read_lef init_floorplan6.lef
read_verilog reg1.v
read_liberty liberty1.lib
link_design top
initialize_floorplan -die_area "0 0 1000 1000" \
  -core_area "100 100 900 900" \
  -site site1
auto_place_pins M1

set def_file [make_result_file init_floorplan6.def]
write_def $def_file
diff_files $def_file init_floorplan6.defok
