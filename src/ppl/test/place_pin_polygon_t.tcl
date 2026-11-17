# place_pin -force_to_die_boundary must keep every pin on the face of a T
# shaped die it was requested at, not move it to the bounding box edges
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def polygon_t.def

# every pin is requested at the middle of the face it is named after
place_pin -pin_name stem_bottom -layer metal2 -location {30 0} \
  -force_to_die_boundary
place_pin -pin_name stem_right -layer metal3 -location {40 15} \
  -force_to_die_boundary
place_pin -pin_name bar_bottom_right -layer metal2 -location {50 30} \
  -force_to_die_boundary
place_pin -pin_name bar_right -layer metal3 -location {60 37.5} \
  -force_to_die_boundary
place_pin -pin_name bar_top -layer metal2 -location {30 45} \
  -force_to_die_boundary
place_pin -pin_name bar_left -layer metal3 -location {0 37.5} \
  -force_to_die_boundary
place_pin -pin_name bar_bottom_left -layer metal2 -location {10 30} \
  -force_to_die_boundary
place_pin -pin_name stem_left -layer metal3 -location {20 15} \
  -force_to_die_boundary

set def_file [make_result_file place_pin_polygon_t.def]
write_def $def_file
diff_files place_pin_polygon_t.defok $def_file
