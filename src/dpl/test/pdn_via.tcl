source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def pdn_via.def

# Exercise physical supply-via conflicts with the diamond legalizer.
detailed_placement -pdn_aware -use_diamond_legalizer
check_placement

set def_file [make_result_file pdn_via.def]
write_def $def_file
check "placement DEF" {diff_files pdn_via.defok $def_file} 0

# A later manual move beneath the via must report a supply-via failure.
set block [ord::get_db_block]
[$block findInst blocked] setLocation 3800 5600
set check_failed [catch { check_placement } check_message]
check "manual supply-via conflict rejected" {
  expr {$check_failed && [string match "*DPL-0033*" $check_message]}
} 1

set supply_via_markers 0
foreach category [$block getMarkerCategories] {
  if { [$category getName] != "DPL" } {
    continue
  }
  foreach subcategory [$category getMarkerCategories] {
    if { [$subcategory getName] == "Fixed_supply_via_failures" } {
      set supply_via_markers [llength [$subcategory getMarkers]]
    }
  }
}
check "supply-via marker count" {expr {$supply_via_markers}} 1

exit_summary
