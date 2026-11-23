# report_placement_density over the core and regions of it
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def

# The density is kept per GCell, which needs the routing layers.
catch { report_placement_density } error
puts $error
set_routing_layers -signal metal2-metal10

# The core runs from 10.07 x 11.2 to 90.25 x 91.  The GCells are squares of
# 2.85 microns, 15 metal2 pitches, from the die origin.  The FIRM
# FILLCELL_X1 cells at the row ends are fillers, which do not count.
report_placement_density
# A region of whole GCells gets their exact instance area.
report_placement_density -region {22.8 22.8 45.6 45.6}
# Elsewhere a GCell the region covers in part adds that share of its area.
report_placement_density -region {20 20 40 40}
# The same region, given by its other two corners.
report_placement_density -region {40 20 20 40}

# The density follows the placement as it changes: an INV_X2, 0.57 x 1.4,
# moved into the top right GCell of the core.
set block [ord::get_db_block]
set top_right {88.35 88.35 90.25 91}
report_placement_density -region $top_right
set inst [$block findInst _440_]
$inst setLocation [ord::microns_to_dbu 89] [ord::microns_to_dbu 89]
report_placement_density -region $top_right
# Fixed, it still counts, as in the heat map.
$inst setPlacementStatus FIRM
report_placement_density -region $top_right
# Unplaced, it does not.
$inst setPlacementStatus UNPLACED
report_placement_density -region $top_right
# Each report counts the database as it stands, so it sees edits odb has no
# callback for.  Placed again, with a hard halo 0.2 microns wide, the INV_X2
# blocks a ring around it.
$inst setPlacementStatus PLACED
set halo [ord::microns_to_dbu 0.2]
$inst setHalo $halo $halo $halo $halo 0
report_placement_density -region $top_right

# Only the part of a region inside the core counts.
report_placement_density -region {0 0 20 20}
report_placement_density -region {0 0 5 5}

# A hard placement blockage over the left half of these 2 x 2 GCells is
# blocked area.  A soft one over the right half only binds global
# placement, so it is not.
set gcells {11.4 11.4 17.1 17.1}
report_placement_density -region $gcells
odb::dbBlockage_create $block \
  [ord::microns_to_dbu 11.4] [ord::microns_to_dbu 11.4] \
  [ord::microns_to_dbu 14.25] [ord::microns_to_dbu 17.1]
set soft [odb::dbBlockage_create $block \
  [ord::microns_to_dbu 14.25] [ord::microns_to_dbu 11.4] \
  [ord::microns_to_dbu 17.1] [ord::microns_to_dbu 17.1]]
$soft setSoft
report_placement_density -region $gcells

catch { report_placement_density -region {20 20 20 40} } error
puts $error
catch { report_placement_density -region {1 2 3} } error
puts $error
catch { report_placement_density -region {1 2 3 abc} } error
puts $error
catch { report_placement_density 1 2 3 4 } error
puts $error
