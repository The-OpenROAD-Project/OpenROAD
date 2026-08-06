# place_pins must keep min spacing from special net via landing pads close to
# the die boundary, even when no wire shape covers them
source "helpers.tcl"

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
for { set x 98000 } { $x <= 106000 } { incr x 380 } {
  odb::dbSBox_create $swire $via $x 295790 "STRIPE"
}

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
  -min_distance 0.12

# count signal pin shapes overlapping or too close to the via pads
proc count_via_pad_violations { } {
  set block [ord::get_db_block]
  set violations 0
  foreach bterm [$block getBTerms] {
    if { [$bterm getSigType] == "POWER" } {
      continue
    }
    foreach bpin [$bterm getBPins] {
      foreach box [$bpin getBoxes] {
        set layer [$box getTechLayer]
        set spacing [$layer getSpacing]
        if { [$layer getName] != "metal2" } {
          continue
        }
        for { set x 98000 } { $x <= 106000 } { incr x 380 } {
          set dx [expr {
            max(0, max($x - 140 - [$box xMax], [$box xMin] - ($x + 140)))
          }]
          set dy [expr {
            max(0, max(295650 - [$box yMax], [$box yMin] - 295930))
          }]
          if { $dx < $spacing && $dy < $spacing } {
            puts "pin [$bterm getName] at\
              ([ord::dbu_to_microns [$box xMin]]\
              [ord::dbu_to_microns [$box yMin]])\
              ([ord::dbu_to_microns [$box xMax]]\
              [ord::dbu_to_microns [$box yMax]]) within\
              [ord::dbu_to_microns [expr { max($dx, $dy) }]]um of via pad at\
              [ord::dbu_to_microns $x]um"
            incr violations
          }
        }
      }
    }
  }
  return $violations
}

puts "pin to via pad violations: [count_via_pad_violations]"
