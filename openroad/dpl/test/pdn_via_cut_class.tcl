# Fixed supply via cuts follow the LEF58 cut-class spacing table as drt reads
# it: without parallel run length the 200 DBU entry is measured between
# centres (CENTERANDEDGE), with it the 120 DBU entry between edges.  A single
# edge-to-edge 200 DBU circle would move aligned_legal and diagonal_legal.
source "helpers.tcl"
read_lef pdn_via_cut_class.lef
read_def pdn_via_cut_class.def

detailed_placement -pdn_aware
check_placement

set block [ord::get_db_block]
# Aligned above, 140 DBU edge gap >= 120.
check "aligned cut beyond its edge spacing stays" {
  [$block findInst aligned_legal] getLocation
} "2000 0"
# Aligned above, 100 DBU edge gap < 120.
check "aligned cut within its edge spacing moves" {
  expr {[lindex [[$block findInst aligned_close] getLocation] 0] != 4000}
} "1"
# Diagonal, 170 DBU between edges but 226 between centres >= 200.
check "diagonal cut beyond its centre spacing stays" {
  [$block findInst diagonal_legal] getLocation
} "6000 0"
# Diagonal, 198 DBU between centres < 200; the nearest legal site is left.
check "diagonal cut within its centre spacing moves" {
  [$block findInst diagonal_close] getLocation
} "7800 0"
exit_summary
