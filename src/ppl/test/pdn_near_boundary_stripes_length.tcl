# place_pins must keep min spacing from near-boundary PDN stripes when the pin
# length is not aligned to the manufacturing grid, since the created pins are
# rounded up to it
source "helpers.tcl"

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

# count signal pin shapes overlapping or too close to the stripe
proc count_stripe_violations { } {
  set block [ord::get_db_block]
  set violations 0
  foreach bterm [$block getBTerms] {
    if { [$bterm getSigType] == "POWER" || [$bterm getSigType] == "GROUND" } {
      continue
    }
    foreach bpin [$bterm getBPins] {
      foreach box [$bpin getBoxes] {
        set layer [$box getTechLayer]
        set spacing [$layer getSpacing]
        if { [$layer getName] != "metal2" } {
          continue
        }
        set dx [expr {
          max(0, max(20000 - [$box xMax], [$box xMin] - 280000))
        }]
        set dy [expr {
          max(0, max(291857 - [$box yMax], [$box yMin] - 293857))
        }]
        if { $dx < $spacing && $dy < $spacing } {
          puts "pin [$bterm getName] at\
            ([ord::dbu_to_microns [$box xMin]]\
            [ord::dbu_to_microns [$box yMin]])\
            ([ord::dbu_to_microns [$box xMax]]\
            [ord::dbu_to_microns [$box yMax]]) within\
            [ord::dbu_to_microns [expr { max($dx, $dy) }]]um of the stripe"
          incr violations
        }
      }
    }
  }
  return $violations
}

puts "pin to stripe violations: [count_stripe_violations]"
