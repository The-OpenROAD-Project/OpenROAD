# place_pins must keep min spacing from routing obstructions at the die edges
source "helpers.tcl"

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

# count signal pin shapes overlapping or too close to the obstruction
proc count_obstruction_violations { } {
  set block [ord::get_db_block]
  set violations 0
  foreach bterm [$block getBTerms] {
    foreach bpin [$bterm getBPins] {
      foreach box [$bpin getBoxes] {
        set layer [$box getTechLayer]
        set spacing [$layer getSpacing]
        if { [$layer getName] != "metal2" } {
          continue
        }
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
            [ord::dbu_to_microns [expr { max($dx, $dy) }]]um of the obstruction"
          incr violations
        }
      }
    }
  }
  return $violations
}

puts "pin to obstruction violations: [count_obstruction_violations]"
