# report_placement_density over the core and regions of it
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def

# The core runs from 10.07 x 11.2 to 90.25 x 91.  Global placement has
# published no bins, so the density is kept in the heat map's default bins
# of 10 microns from the lower left corner of the core.  The FIRM
# FILLCELL_X1 cells at the row ends are fillers, which do not count.
report_placement_density
# A region of whole bins gets their exact instance area.
report_placement_density -region {20.07 21.2 40.07 41.2}
# Elsewhere a bin the region covers in part adds that share of its area.
report_placement_density -region {20 20 40 40}
# The same region, given by its other two corners.
report_placement_density -region {40 20 20 40}

# The density follows the placement as it changes: an INV_X2, 0.57 x 1.4,
# moved into the top right bins.
set block [ord::get_db_block]
set top_right {80.07 81.2 90.25 91}
report_placement_density -region $top_right
set inst [$block findInst _440_]
$inst setLocation [ord::microns_to_dbu 85] [ord::microns_to_dbu 85]
report_placement_density -region $top_right
# Fixed, it still counts, as in the heat map.
$inst setPlacementStatus FIRM
report_placement_density -region $top_right
# Unplaced, it does not.
$inst setPlacementStatus UNPLACED
report_placement_density -region $top_right

# Only the part of a region inside the core counts.
report_placement_density -region {0 0 20 20}
report_placement_density -region {0 0 5 5}

catch { report_placement_density -region {20 20 20 40} } error
puts $error
catch { report_placement_density -region {1 2 3} } error
puts $error
catch { report_placement_density -region {1 2 3 abc} } error
puts $error
catch { report_placement_density 1 2 3 4 } error
puts $error
