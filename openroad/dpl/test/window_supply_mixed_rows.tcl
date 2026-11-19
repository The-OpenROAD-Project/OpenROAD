# A legal placement on rows of mixed height must not be stopped: three rows
# of 12 sites, single, double and single height, filled by cells of the
# matching height.
source "helpers.tcl"
read_lef window_supply_mixed_rows.lef
read_def window_supply_mixed_rows.def

check_placement
detailed_placement -use_diamond_legalizer
check_placement
check "legal mixed-height placement is left legal" { expr { 1 } } 1
exit_summary
