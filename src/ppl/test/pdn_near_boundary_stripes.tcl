# place_pins must keep min spacing from PDN stripes close to the die boundary,
# even when they do not touch it, since pin shapes extend into the die
source "helpers.tcl"

# slot count and HPWL change once boundary PDN shapes block slots
suppress_message PPL 1
suppress_message PPL 12

read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def

set block [ord::get_db_block]
set metal2 [[ord::get_db_tech] findLayer metal2]

# stripe without pins ending 0.05um inside the top edge (die top y=296000)
set net [odb::dbNet_create $block "VDD"]
$net setSpecial
$net setSigType POWER
set swire [odb::dbSWire_create $net "ROUTED"]
odb::dbSBox_create $swire $metal2 22140 295340 273640 295900 "STRIPE"

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
  -min_distance 0.12

# count signal pin shapes overlapping or too close to special net wires
proc count_pin_stripe_violations { } {
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
        foreach net [$block getNets] {
          if { ![$net isSpecial] } {
            continue
          }
          foreach swire [$net getSWires] {
            foreach sbox [$swire getWires] {
              set slayer [$sbox getTechLayer]
              if { $slayer == "NULL" || $slayer != $layer } {
                continue
              }
              set dx [expr {
                max(0, max([$sbox xMin] - [$box xMax], [$box xMin] - [$sbox xMax]))
              }]
              set dy [expr {
                max(0, max([$sbox yMin] - [$box yMax], [$box yMin] - [$sbox yMax]))
              }]
              if { $dx < $spacing && $dy < $spacing } {
                puts "pin [$bterm getName] on layer [$layer getName] at\
                  ([ord::dbu_to_microns [$box xMin]]\
                  [ord::dbu_to_microns [$box yMin]])\
                  ([ord::dbu_to_microns [$box xMax]]\
                  [ord::dbu_to_microns [$box yMax]]) within\
                  [ord::dbu_to_microns [expr { max($dx, $dy) }]]um of\
                  [$net getName] stripe"
                incr violations
              }
            }
          }
        }
      }
    }
  }
  return $violations
}

puts "pin to near boundary stripe violations: [count_pin_stripe_violations]"
