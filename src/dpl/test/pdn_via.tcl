source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def pdn_via.def

# Opt in to physical supply-via conflicts; the diamond legalizer is an
# independent placement strategy and must preserve the same physical result.
detailed_placement -pdn_aware -use_diamond_legalizer
check_placement

set block [ord::get_db_block]
check "blocked location" {[$block findInst blocked] getLocation} "4180 5600"
check "same-net location" {[$block findInst same_net] getLocation} "7600 5600"
check "signal-pin blocked location" {
  [$block findInst signal_blocked] getLocation
} "11780 5600"
check "signal-pin same-net location" {
  [$block findInst signal_same_net] getLocation
} "15200 5600"

# A later manual move beneath the via must be reported as a supply-via failure,
# independently from the blocked-layer category.
[$block findInst blocked] setLocation 3800 5600
set check_failed [catch {check_placement} check_message]
check "manual supply-via conflict rejected" {
  expr {$check_failed && [string match "*DPL-0033*" $check_message]}
} 1

set supply_via_markers 0
foreach category [$block getMarkerCategories] {
  if {[$category getName] != "DPL"} {
    continue
  }
  foreach subcategory [$category getMarkerCategories] {
    if {[$subcategory getName] == "Fixed_supply_via_failures"} {
      set supply_via_markers [llength [$subcategory getMarkers]]
    }
  }
}
check "supply-via marker count" {expr {$supply_via_markers}} 1

exit_summary
