# place_pin -force_to_die_boundary must keep every pin on the face of a U
# shaped die it was requested at, including the notch faces that face the
# outer faces
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def polygon_u.def

# every pin is requested at the middle of the face it is named after
place_pin -pin_name bottom -layer metal2 -location {30 0} \
  -force_to_die_boundary
place_pin -pin_name right -layer metal3 -location {60 20} \
  -force_to_die_boundary
place_pin -pin_name right_arm_top -layer metal2 -location {50 40} \
  -force_to_die_boundary
place_pin -pin_name notch_right -layer metal3 -location {40 27.5} \
  -force_to_die_boundary
place_pin -pin_name notch_floor -layer metal2 -location {30 15} \
  -force_to_die_boundary
place_pin -pin_name notch_left -layer metal3 -location {20 27.5} \
  -force_to_die_boundary
place_pin -pin_name left_arm_top -layer metal2 -location {10 40} \
  -force_to_die_boundary
place_pin -pin_name left -layer metal3 -location {0 20} \
  -force_to_die_boundary

set def_file [make_result_file place_pin_polygon_u.def]
write_def $def_file
diff_files place_pin_polygon_u.defok $def_file
