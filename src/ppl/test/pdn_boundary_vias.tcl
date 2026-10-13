# place_pins must keep min spacing from special net via landing pads close to
# the die boundary, even when no wire shape covers them
source "helpers.tcl"
source "pdn_helpers.tcl"

# slot count and HPWL change once boundary PDN shapes block slots
suppress_message PPL 1
suppress_message PPL 12

read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def

set block [ord::get_db_block]
set tech [ord::get_db_tech]
set via [$tech findVia "via1_0"]

# naked vias near the top edge; via1_0 metal2 enclosure is 0.07um around the
# origin, so the pads span y 295650-295930, within reach of the pins
set net [odb::dbNet_create $block "VDD"]
$net setSpecial
$net setSigType POWER
set swire [odb::dbSWire_create $net "ROUTED"]
set via_xs {}
for { set x 98000 } { $x <= 106000 } { incr x 380 } {
  odb::dbSBox_create $swire $via $x 295790 "STRIPE"
  lappend via_xs $x
}

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
  -min_distance 0.12

set via_rects {}
foreach x $via_xs {
  lappend via_rects [list [expr { $x - 140 }] 295650 [expr { $x + 140 }] 295930]
}
puts "pin to via pad violations: [count_rect_violations metal2 $via_rects spacing]"
