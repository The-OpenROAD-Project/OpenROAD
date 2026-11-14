# place_pin -force_to_die_boundary must keep every pin on the face of an L
# shaped die it was requested at, not move it to the bounding box edges
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def polygon_l.def

# every pin is requested at the middle of the face it is named after
place_pin -pin_name bottom -layer metal2 -location {20 0} \
  -force_to_die_boundary
place_pin -pin_name right -layer metal3 -location {40 10} \
  -force_to_die_boundary
place_pin -pin_name inner_top -layer metal2 -location {30 20} \
  -force_to_die_boundary
place_pin -pin_name inner_right -layer metal3 -location {20 30} \
  -force_to_die_boundary
place_pin -pin_name top -layer metal2 -location {10 40} \
  -force_to_die_boundary
place_pin -pin_name left -layer metal3 -location {0 20} \
  -force_to_die_boundary
# below the end of the inner right face, which is still the nearest face a
# metal3 pin reaches, so the pin starts at the first track on it
place_pin -pin_name corner -layer metal3 -location {19 19} \
  -force_to_die_boundary

set def_file [make_result_file place_pin_polygon_l.def]
write_def $def_file
diff_files place_pin_polygon_l.defok $def_file
