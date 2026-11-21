# A cell that fence preprocessing has already moved further than its window
# must not be charged to the window around its start: prePlace moves wide
# from site 19, on the fence, to site 10, with a window of one site.
source "helpers.tcl"
read_lef window_supply_fence.lef
read_def window_supply_fence.def

# Sites and rows are one micron, so this is +/- one site and one row.
detailed_placement -use_diamond_legalizer -max_displacement {1 1}
check_placement
set block [ord::get_db_block]
set wide [$block findInst wide]
check "wide cell moved off the fence" {
  expr { [$wide getLocation] eq {20000 4000} }
} 1
exit_summary
