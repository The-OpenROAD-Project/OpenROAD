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

# supply pin whose first bpin is placed and second is firm
set pg2_net [odb::dbNet_create $block "VDDM"]
set pg2_term [odb::dbBTerm_create $pg2_net "VDDM"]
$pg2_term setSigType POWER
set pg2_firm [odb::dbBPin_create $pg2_term]
odb::dbBox_create $pg2_firm $metal2 240000 295440 260000 296000
$pg2_firm setPlacementStatus FIRM
set pg2_placed [odb::dbBPin_create $pg2_term]
odb::dbBox_create $pg2_placed $metal2 20000 295440 30000 296000
$pg2_placed setPlacementStatus PLACED

place_pin -pin_name req_msg\[0\] -layer metal2 -location {51 148} \
  -pin_size {0.14 5.0} -force_to_die_boundary
place_pin -pin_name req_msg\[1\] -layer metal2 -location {105 148} \
  -force_to_die_boundary
place_pin -pin_name req_msg\[2\] -layer metal2 -location {125 148} \
  -force_to_die_boundary

# firm signal pin, visible to place_pin only through stale run keepouts
set sig_bpin [odb::dbBPin_create [$block findBTerm {resp_msg\[15\]}]]
odb::dbBox_create $sig_bpin $metal2 160000 295440 160280 296000
$sig_bpin setPlacementStatus FIRM

# an aborted run must not leave its keepouts behind for place_pin
catch {
  place_pins -hor_layers metal3 -ver_layers metal2 -min_distance 100
}

# explicit request over the firm signal pin, must stay where asked
place_pin -pin_name req_msg\[3\] -layer metal2 -location {80.025 148} \
  -force_to_die_boundary

puts "pin to stripe violations: [count_pdn_shape_violations spacing]"
puts "pin violations against PG pins: [count_pg_pin_violations spacing]"
