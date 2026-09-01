# place_pin -force_to_die_boundary must pad the blocking shapes with the
# requested -pin_size and keep min spacing to fixed power/ground pins
source "helpers.tcl"
source "pdn_helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def

set block [ord::get_db_block]
set metal2 [[ord::get_db_tech] findLayer metal2]

# stripe recessed from the top edge, only reachable by a long pin
set net [odb::dbNet_create $block "VDD"]
$net setSpecial
$net setSigType POWER
set swire [odb::dbSWire_create $net "ROUTED"]
odb::dbSBox_create $swire $metal2 100000 294000 104000 295000 "STRIPE"

# fixed power pin on a top edge segment
set pg_net [odb::dbNet_create $block "VDDT"]
set pg_term [odb::dbBTerm_create $pg_net "VDDT"]
$pg_term setSigType POWER
set pg_pin [odb::dbBPin_create $pg_term]
odb::dbBox_create $pg_pin $metal2 200000 295440 220000 296000
$pg_pin setPlacementStatus FIRM

place_pin -pin_name req_msg\[0\] -layer metal2 -location {51 148} \
  -pin_size {0.14 5.0} -force_to_die_boundary
place_pin -pin_name req_msg\[1\] -layer metal2 -location {105 148} \
  -force_to_die_boundary

puts "pin to stripe violations: [count_pdn_shape_violations spacing]"
puts "pin violations against PG pins: [count_pg_pin_violations spacing]"
