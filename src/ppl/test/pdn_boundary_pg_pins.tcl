# place_pins must keep min spacing from fixed power/ground pins on the die edge
source "helpers.tcl"

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

# count signal pin shapes overlapping or too close to fixed PG pins
proc count_pg_pin_violations { } {
  set block [ord::get_db_block]
  set violations 0
  set pg_boxes {}
  foreach bterm [$block getBTerms] {
    if { [$bterm getSigType] != "POWER" && [$bterm getSigType] != "GROUND" } {
      continue
    }
    foreach bpin [$bterm getBPins] {
      foreach box [$bpin getBoxes] {
        lappend pg_boxes [list [$bterm getName] $box]
      }
    }
  }
  foreach bterm [$block getBTerms] {
    if { [$bterm getSigType] == "POWER" || [$bterm getSigType] == "GROUND" } {
      continue
    }
    foreach bpin [$bterm getBPins] {
      foreach box [$bpin getBoxes] {
        set layer [$box getTechLayer]
        set spacing [$layer getSpacing]
        foreach pg_box_pair $pg_boxes {
          lassign $pg_box_pair pg_name pg_box
          if { [$pg_box getTechLayer] != $layer } {
            continue
          }
          set dx [expr {
            max(0, max([$pg_box xMin] - [$box xMax], [$box xMin] - [$pg_box xMax]))
          }]
          set dy [expr {
            max(0, max([$pg_box yMin] - [$box yMax], [$box yMin] - [$pg_box yMax]))
          }]
          if { $dx < $spacing && $dy < $spacing } {
            puts "pin [$bterm getName] on layer [$layer getName] at\
              ([ord::dbu_to_microns [$box xMin]]\
              [ord::dbu_to_microns [$box yMin]])\
              ([ord::dbu_to_microns [$box xMax]]\
              [ord::dbu_to_microns [$box yMax]]) within\
              [ord::dbu_to_microns [expr { max($dx, $dy) }]]um of $pg_name pin"
            incr violations
          }
        }
      }
    }
  }
  return $violations
}

puts "pin violations against PG pins: [count_pg_pin_violations]"
