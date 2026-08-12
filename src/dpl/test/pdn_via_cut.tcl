# Fixed supply via CUT constituents use Euclidean spacing.  The routing
# constituents are intentionally tiny, so these cases isolate cut spacing
# rather than a metal keepout.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef pdn_via_cut.lef
read_def pdn_via_cut.def

detailed_placement -pdn_aware
check_placement

set block [ord::get_db_block]
check "cut constituent conflict location" {
  [$block findInst cut_conflict] getLocation
} "2200 0"
check "diagonal below cut spacing location" {
  [$block findInst diagonal_below] getLocation
} "3800 0"
check "diagonal equal to cut spacing" {
  [$block findInst diagonal_equal] getLocation
} "6000 0"
check "diagonal above cut spacing" {
  [$block findInst diagonal_above] getLocation
} "8000 0"
exit_summary
