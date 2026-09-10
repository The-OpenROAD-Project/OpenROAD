# place_pins must not place pins over boundary PDN stripes on polygon dies
source "helpers.tcl"
source "pdn_helpers.tcl"

# slot count and HPWL change once boundary PDN shapes block slots
suppress_message PPL 1
suppress_message PPL 12

read_lef Nangate45/Nangate45.lef
read_def gcd_polygon_pre_ppl-tcl.def

set block [ord::get_db_block]
set metal2 [[ord::get_db_tech] findLayer metal2]

# stripe without pins touching the bottom edge of the polygon stem
set net [$block findNet "VDD"]
set swire [odb::dbSWire_create $net "ROUTED"]
odb::dbSBox_create $swire $metal2 70000 0 74000 20000 "STRIPE"

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
  -min_distance 0.12

puts "pin to stripe violations: [count_pdn_shape_violations spacing]"
