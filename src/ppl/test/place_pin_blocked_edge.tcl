# place_pin must not push a pin outside the die area when the whole edge is
# blocked, it must report the failure instead
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def one_net.def

set block [ord::get_db_block]
set metal2 [[ord::get_db_tech] findLayer metal2]

# strap without pins covering the entire top edge
set net [odb::dbNet_create $block "VDD"]
$net setSpecial
$net setSigType POWER
set swire [odb::dbSWire_create $net "ROUTED"]
odb::dbSBox_create $swire $metal2 0 19000 20000 20000 "STRIPE"

catch {
  place_pin -pin_name in1 -layer metal2 -location {5 10} -force_to_die_boundary
} error
puts $error

# the pin must not be placed outside the die area
set die_area [$block getDieArea]
foreach bpin [[$block findBTerm in1] getBPins] {
  foreach box [$bpin getBoxes] {
    if {
      [$box xMin] < [$die_area xMin] || [$box xMax] > [$die_area xMax]
      || [$box yMin] < [$die_area yMin] || [$box yMax] > [$die_area yMax]
    } {
      puts "pin in1 placed outside the die area:\
        ([$box xMin] [$box yMin]) ([$box xMax] [$box yMax])"
    }
  }
}
