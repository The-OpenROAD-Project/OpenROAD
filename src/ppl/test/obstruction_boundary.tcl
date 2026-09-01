# place_pins must keep min spacing from routing obstructions at the die edges
source "helpers.tcl"
source "pdn_helpers.tcl"

# slot count and HPWL change once boundary shapes block slots
suppress_message PPL 1
suppress_message PPL 12

read_lef Nangate45/Nangate45.lef
read_def gcd_placed.def

set block [ord::get_db_block]
set metal2 [[ord::get_db_tech] findLayer metal2]

# long pins reach the wide shape rows of the spacing table
set_pin_length -hor_length 1.0 -ver_length 1.0

# routing obstruction touching the top edge
set obstruction_rect {100000 292000 104000 296000}
odb::dbObstruction_create $block $metal2 {*}$obstruction_rect

# obstruction with a min spacing rule larger than the layer spacing
set min_spacing_rect {150000 292000 154000 296000}
set spacing_obs [odb::dbObstruction_create $block $metal2 {*}$min_spacing_rect]
$spacing_obs setMinSpacing 5000

# narrow obstruction with an effective width selecting the wide spacing row
set eff_width_rect {160400 295800 164400 296000}
set eff_obs [odb::dbObstruction_create $block $metal2 {*}$eff_width_rect]
$eff_obs setEffectiveWidth 2000

# fill and slot only blockages allow routing metal, so pins may use them
set blockage_rect {110000 292000 200000 296000}
set fill_obs [odb::dbObstruction_create $block $metal2 {*}$blockage_rect]
$fill_obs setFillObstruction
set slot_obs [odb::dbObstruction_create $block $metal2 {*}$blockage_rect]
$slot_obs setSlotObstruction

place_pins -hor_layers metal3 -ver_layers metal2 -corner_avoidance 0 \
  -min_distance 0.12

proc count_pins_in_rect { llx lly urx ury } {
  set count 0
  foreach bterm [[ord::get_db_block] getBTerms] {
    foreach bpin [$bterm getBPins] {
      foreach box [$bpin getBoxes] {
        if {
          [$box xMin] < $urx && $llx < [$box xMax]
          && [$box yMin] < $ury && $lly < [$box yMax]
        } {
          incr count
        }
      }
    }
  }
  return $count
}

puts "pin to obstruction violations:\
  [count_rect_violations metal2 [list $obstruction_rect] spacing]"
puts "pin to min spacing obstruction violations:\
  [count_rect_violations metal2 [list $min_spacing_rect] 5000]"
puts "pin to effective width obstruction violations:\
  [count_rect_violations metal2 [list $eff_width_rect] \
  [$metal2 getSpacing 2000 2000]]"
puts "pins over the fill and slot blockages:\
  [count_pins_in_rect {*}$blockage_rect]"
