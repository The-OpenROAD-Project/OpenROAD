# place_pin -force_to_die_boundary must move a pin away from PDN shapes on the
# inner faces of a polygon die, without leaving the face it was requested at
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def polygon_u.def

set block [ord::get_db_block]
set tech [ord::get_db_tech]
set net [odb::dbNet_create $block "VDD"]
$net setSpecial
$net setSigType POWER
set swire [odb::dbSWire_create $net "ROUTED"]
# stripe on the right half of the notch floor, the nearest free position past
# it is off the face, to the right of the notch
odb::dbSBox_create $swire [$tech findLayer metal2] 60000 26000 80000 30000 \
  "STRIPE"
# strap along the whole left wall of the notch
odb::dbSBox_create $swire [$tech findLayer metal3] 36000 30000 40000 80000 \
  "STRIPE"

place_pin -pin_name notch_floor -layer metal2 -location {38 15} \
  -force_to_die_boundary

catch {
  place_pin -pin_name notch_left -layer metal3 -location {20 27.5} \
    -force_to_die_boundary
} error
puts $error

set def_file [make_result_file place_pin_polygon_blocked.def]
write_def $def_file
diff_files place_pin_polygon_blocked.defok $def_file
