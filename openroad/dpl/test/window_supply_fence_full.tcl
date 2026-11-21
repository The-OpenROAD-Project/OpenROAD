# Control for window_supply_fence: the cell fence preprocessing moved is left
# out, the check is not. 16 cells of 2 sites stacked on one site need 32
# sites, and a window of one site and one row reaches 24.
source "helpers.tcl"
read_lef window_supply_fence.lef
read_def window_supply_fence_full.def

set failed [catch {
  detailed_placement -use_diamond_legalizer -max_displacement {1 1}
} message]
check "overfull window stops the diamond search" {
  expr { $failed && [string match "*DPL-1107*" $message] }
} 1
exit_summary
