# Supply vias are ignored by ordinary detailed placement and after PDN-aware
# mode is disabled on the same block.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def fixed_via.def

detailed_placement
check_placement

set block [ord::get_db_block]
check "default-off supply-via location" {
  [$block findInst blocked] getLocation
} "3800 5600"
check "default-off same-net location" {
  [$block findInst same_net] getLocation
} "7600 5600"
check "default-off signal-via location" {
  [$block findInst signal_blocked] getLocation
} "11400 5600"
check "default-off signal-net location" {
  [$block findInst signal_same_net] getLocation
} "15200 5600"

detailed_placement -pdn_aware
[$block findInst blocked] setLocation 3800 5600
detailed_placement
check_placement
check "default-off location after PDN-aware placement" {
  [$block findInst blocked] getLocation
} "3800 5600"
exit_summary
