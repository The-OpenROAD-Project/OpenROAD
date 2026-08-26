# place_pin -force_to_die_boundary must move pins away from boundary PDN
# stripes instead of placing them on top
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def

set block [ord::get_db_block]
set metal2 [[ord::get_db_tech] findLayer metal2]

# stripe without pins on the top edge
set net [odb::dbNet_create $block "VDD"]
$net setSpecial
$net setSigType POWER
set swire [odb::dbSWire_create $net "ROUTED"]
odb::dbSBox_create $swire $metal2 100000 292000 104000 296000 "STRIPE"

place_pin -pin_name req_msg\[0\] -layer metal2 -location {51 148} \
  -force_to_die_boundary
place_pin -pin_name req_msg\[1\] -layer metal2 -location {50.5 148} \
  -force_to_die_boundary

# count pin shapes overlapping or too close to the stripe
proc count_stripe_violations { } {
  set block [ord::get_db_block]
  set violations 0
  foreach name [list {req_msg\[0\]} {req_msg\[1\]}] {
    set bterm [$block findBTerm $name]
    foreach bpin [$bterm getBPins] {
      foreach box [$bpin getBoxes] {
        set layer [$box getTechLayer]
        set spacing [$layer getSpacing]
        set dx [expr {
          max(0, max(100000 - [$box xMax], [$box xMin] - 104000))
        }]
        set dy [expr {
          max(0, max(292000 - [$box yMax], [$box yMin] - 296000))
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
