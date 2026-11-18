# check_placement -placeable validates the movable cells while ignoring
# violations of fixed instances, which still block the movable cells.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef extra.lef
read_def check_fixed_only.def

set block [ord::get_db_block]

# Misalign the fixed tapcell by half a site and move the stale buffer to a
# legal location; the full check fails on the tapcell ...
set tap [$block findInst tap1]
$tap setPlacementStatus PLACED
$tap setLocation 3990 2800
$tap setPlacementStatus FIRM
set buf [$block findInst _277_]
$buf setLocation 11400 2800
catch { check_placement -verbose } error
puts $error

# ... but is ignored when checking only the placeable cells.
check_placement -placeable -verbose
puts "placeable: pass"

# A buffer placed over the fixed macro is still reported.
$buf setLocation 7600 5600
catch { check_placement -placeable -verbose } error
puts $error
