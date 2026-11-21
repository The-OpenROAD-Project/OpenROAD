# Control for window_supply_mixed_rows: on the same rows, twice as long, 13
# cells of 2 sites stacked on one site need 26 um^2, and a window of one
# site and one row reaches 20 um^2.
source "helpers.tcl"
read_lef window_supply_mixed_rows.lef
read_def window_supply_mixed_rows_full.def

set failed [catch {
  detailed_placement -use_diamond_legalizer -max_displacement {1 1}
} message]
check "overfull mixed-height core stops the diamond search" {
  expr { $failed && [string match "*DPL-1107*" $message] }
} 1
exit_summary
