# place_pins must keep min spacing from routing obstructions at the die edges
source "helpers.tcl"
source "pdn_helpers.tcl"

# slot count and HPWL change once boundary shapes block slots
suppress_message PPL 1
suppress_message PPL 12

read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def

set block [ord::get_db_block]
set metal2 [[ord::get_db_tech] findLayer metal2]

# routing obstruction touching the top edge
odb::dbObstruction_create $block $metal2 100000 292000 104000 296000

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
  -min_distance 0.12

set obstruction_rect [list [list 100000 292000 104000 296000]]
puts "pin to obstruction violations: [count_rect_violations metal2 $obstruction_rect spacing]"
