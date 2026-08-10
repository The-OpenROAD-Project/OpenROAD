# Check strict Euclidean routing spacing for fixed supply via shapes.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef fixed_via_euclidean.lef
read_def fixed_via_euclidean.def

# Opt in to physical supply-via spacing for this Euclidean geometry case.
detailed_placement -pdn_aware
check_placement

set block [ord::get_db_block]

# The axial gap is 100 DBU, below the 130 DBU metal1 spacing.  Both rows are
# blocked at this x location, so the cell moves to the next legal site.
check "axial conflict location" {
  [$block findInst axial] getLocation
} "600 0"

# A keepout ending one DBU before the core boundary is not a conflict and must
# not push the first-site cell out of the row.
check "core-left-boundary location" {
  [$block findInst left_boundary] getLocation
} "0 400"

# Each diagonal axis has a 100 DBU gap, but sqrt(100^2 + 100^2) exceeds the
# 130 DBU spacing.  The initial location remains legal on row 0.
check "diagonal large location" {
  [$block findInst diagonal_large] getLocation
} "4000 0"

# The diagonal gaps are 80 and 100 DBU, so their squared distance is below
# the required spacing squared.  The cell moves away from the via.
check "diagonal small location" {
  [$block findInst diagonal_small] getLocation
} "5800 0"

# Gaps of 50 and 120 DBU are exactly 130 DBU by the 5-12-13 relation.  The
# strict spacing rule allows equality, so the initial row-0 site is retained.
check "diagonal equality location" {
  [$block findInst diagonal_equal] getLocation
} "8000 0"

# A supply pin on the same special net remains exempt from the conflict.
check "same-net location" {
  [$block findInst same_net] getLocation
} "2000 0"

exit_summary
