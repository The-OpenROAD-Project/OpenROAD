# mirrored pins must not land on slots blocked by boundary PDN shapes:
# the matcher avoids them when possible and errors out when not
source "helpers.tcl"

# slot count and HPWL change once boundary PDN shapes block slots
suppress_message PPL 1
suppress_message PPL 12

read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def

set block [ord::get_db_block]
set metal2 [[ord::get_db_tech] findLayer metal2]

# strap on the top edge only, so mirrored slots at the bottom edge stay free
set net [odb::dbNet_create $block "VDD"]
$net setSpecial
$net setSigType POWER
set swire [odb::dbSWire_create $net "ROUTED"]
odb::dbSBox_create $swire $metal2 100000 292000 104000 296000 "STRIPE"

set_io_pin_constraint -mirrored_pins {req_msg\[0\] req_msg\[1\]}
set_io_pin_constraint -region bottom:40-60 -pin_names {req_msg\[0\]}

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
  -min_distance 0.12

# count mirrored pin shapes overlapping the strap
proc count_strap_overlaps { } {
  set block [ord::get_db_block]
  set violations 0
  foreach name [list {req_msg\[0\]} {req_msg\[1\]}] {
    set bterm [$block findBTerm $name]
    foreach bpin [$bterm getBPins] {
      foreach box [$bpin getBoxes] {
        set layer [$box getTechLayer]
        if { [$layer getName] != "metal2" } {
          continue
        }
        if {
          [$box xMin] < 104000 && 100000 < [$box xMax] && [$box yMin] < 296000
          && 292000 < [$box yMax]
        } {
          puts "pin [$bterm getName] overlaps the VDD strap"
          incr violations
        }
      }
    }
  }
  return $violations
}

puts "mirrored pin overlaps with strap: [count_strap_overlaps]"

# a region whose mirrored slots are all blocked must error, not overlap
clear_io_pin_constraints
set_io_pin_constraint -mirrored_pins {req_msg\[2\] req_msg\[3\]}
set_io_pin_constraint -region bottom:50.2-51.8 -pin_names {req_msg\[2\]}

catch {
  place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
    -min_distance 0.12
} error
puts $error
