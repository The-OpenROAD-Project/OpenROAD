# place_pins must keep min spacing from near-boundary PDN stripes when the pin
# length is not aligned to the manufacturing grid, since the created pins are
# rounded up to it
source "helpers.tcl"
source "pdn_helpers.tcl"

# slot count and HPWL change once boundary PDN shapes block slots
suppress_message PPL 1
suppress_message PPL 12

read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def

set block [ord::get_db_block]
set metal2 [[ord::get_db_tech] findLayer metal2]

# stripe just outside the reach of a 1.001um pin before mfg-grid rounding;
# the actual pins round up to 1.005um and reach it
set net [odb::dbNet_create $block "VDD"]
$net setSpecial
$net setSigType POWER
set swire [odb::dbSWire_create $net "ROUTED"]
odb::dbSBox_create $swire $metal2 20000 291857 280000 293857 "STRIPE"

set_pin_length -ver_length 1.001 -hor_length 1.001

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
  -min_distance 0.12

puts "pin to stripe violations: [count_pdn_shape_violations spacing]"
