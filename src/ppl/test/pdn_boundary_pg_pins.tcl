# place_pins must keep min spacing from fixed power/ground pins on the die edge
source "helpers.tcl"
source "pdn_helpers.tcl"

# slot count and HPWL change once boundary PDN shapes block slots
suppress_message PPL 1
suppress_message PPL 12

read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def

set block [ord::get_db_block]
set metal2 [[ord::get_db_tech] findLayer metal2]

# fixed power pin on the top edge, like the ones created by pdngen; its ends
# sit between routing tracks so the adjacent slot centers fall outside the box
set net [odb::dbNet_create $block "VDD"]
$net setSpecial
$net setSigType POWER
set term [odb::dbBTerm_create $net "VDD"]
$term setSigType POWER
set pin [odb::dbBPin_create $term]
odb::dbBox_create $pin $metal2 22140 295440 273640 296000
$pin setPlacementStatus FIRM

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
  -min_distance 0.12 -exclude left:* -exclude right:* -exclude bottom:*

puts "pin violations against PG pins: [count_pg_pin_violations spacing]"
