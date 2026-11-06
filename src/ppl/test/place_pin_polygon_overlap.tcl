# place_pin -force_to_die_boundary must move a pin away from a pin placed by
# an earlier place_pin call on the same face and layer of a polygon die
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def polygon_u.def

# outer face
place_pin -pin_name notch_floor -layer metal2 -location {30 0} \
  -force_to_die_boundary
place_pin -pin_name notch_left -layer metal2 -location {30 0} \
  -force_to_die_boundary
# inner face
place_pin -pin_name left_arm_top -layer metal2 -location {30 15} \
  -force_to_die_boundary
place_pin -pin_name right_arm_top -layer metal2 -location {30 15} \
  -force_to_die_boundary
# a PLACED pin blocks as well
place_pin -pin_name bottom -layer metal2 -location {50 0} \
  -force_to_die_boundary -placed_status
place_pin -pin_name right -layer metal2 -location {50 0} \
  -force_to_die_boundary

set def_file [make_result_file place_pin_polygon_overlap.def]
write_def $def_file
diff_files place_pin_polygon_overlap.defok $def_file
