# check_placement -fixed_only validates fixed instances and macros while
# ignoring standard cells that are still at stale floorplan-stage locations.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef extra.lef
read_def check_fixed_only.def

# The stale standard cell fails the full check ...
catch { check_placement -verbose } error
puts $error

# ... but is ignored when checking the floorplan.
check_placement -fixed_only -verbose
puts "fixed_only: pass"

# Misalign the fixed tapcell by half a site; the floorplan check now fails.
set tap [[ord::get_db_block] findInst tap1]
$tap setPlacementStatus PLACED
$tap setLocation 3990 2800
$tap setPlacementStatus FIRM
catch { check_placement -fixed_only -verbose } error
puts $error
