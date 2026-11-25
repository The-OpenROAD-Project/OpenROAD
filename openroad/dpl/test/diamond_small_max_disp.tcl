# Diamond legalization with a small -max_displacement (< 20 rows) keeps
# valid sites reachable instead of marking the grid hopeless.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd_replace.def

detailed_placement -use_diamond_legalizer -max_displacement {10 10}
check "check_placement succeeds" {catch {check_placement}} 0

exit_summary
