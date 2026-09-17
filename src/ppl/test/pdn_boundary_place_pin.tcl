# place_pin -force_to_die_boundary must move pins away from boundary PDN
# stripes instead of placing them on top
source "helpers.tcl"
source "pdn_helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def

set block [ord::get_db_block]
set metal2 [[ord::get_db_tech] findLayer metal2]

# stripe without pins on the top edge
set net [odb::dbNet_create $block "VDD"]
$net setSpecial
$net setSigType POWER
set swire [odb::dbSWire_create $net "ROUTED"]
odb::dbSBox_create $swire $metal2 100000 292000 104000 296000 "STRIPE"

place_pin -pin_name req_msg\[0\] -layer metal2 -location {51 148} \
  -force_to_die_boundary
place_pin -pin_name req_msg\[1\] -layer metal2 -location {50.5 148} \
  -force_to_die_boundary

puts "pin to stripe violations: [count_pdn_shape_violations spacing]"
