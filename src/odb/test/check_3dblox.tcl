source "helpers.tcl"

# Suppress noisy standard loading messages and checker warnings
suppress_message ODB 227
suppress_message ODB 128
suppress_message ODB 131
suppress_message ODB 133
suppress_message STA 1171
suppress_message ODB 156
suppress_message ODB 151

# 1. Load clean design
read_3dbx "data/example.3dbx"
set db [ord::get_db]
set top_chip [$db getChip]

# Get design info
set inst1 [$top_chip findChipInst "soc_inst"]
lassign [$inst1 getLoc] x1 y1 z1
set master1 [$inst1 getMasterChip]
set w1 [$master1 getWidth]
set h1 [$master1 getHeight]
set t1 [$master1 getThickness]

set inst2 [$top_chip findChipInst "soc_inst_duplicate"]

# Verify it is clean initially
set category [$top_chip findMarkerCategory "3DBlox"]
if { $category != "NULL" } {
  set overlapping_category [$category findMarkerCategory "Overlapping chips"]
  check "Initial overlap count" { expr { $overlapping_category == "NULL" ? 0 : [$overlapping_category getMarkerCount] } } 0
  set floating_category [$category findMarkerCategory "Floating chips"]
  check "Initial floating count" { expr { $floating_category == "NULL" ? 0 : [$floating_category getMarkerCount] } } 0
}

# 2. Test Partial Overlap
# Move inst2 to partially overlap with inst1
set p [odb::Point3D]
$p set [expr $x1 + $w1 / 4] [expr $y1 + $h1 / 4] [expr $z1 + $t1 / 2]
$inst2 setLoc $p

check_3dblox
set category [$top_chip findMarkerCategory "3DBlox"]
set overlapping_category [$category findMarkerCategory "Overlapping chips"]
check "Partial overlap detected" { expr { $overlapping_category != "NULL" && [$overlapping_category getMarkerCount] > 0 } } 1

# 3. Test Touching (Stacked Exactly)
# Place inst2 exactly on top of inst1
$p set $x1 $y1 [expr $z1 + $t1]
$inst2 setLoc $p

check_3dblox
set category [$top_chip findMarkerCategory "3DBlox"]
set overlapping_category [$category findMarkerCategory "Overlapping chips"]
check "Touching chips no overlap" { expr { $overlapping_category == "NULL" ? 0 : [$overlapping_category getMarkerCount] } } 0
set floating_category [$category findMarkerCategory "Floating chips"]
check "Touching chips not floating" { expr { $floating_category == "NULL" ? 0 : [$floating_category getMarkerCount] } } 0

# 4. Test Vertical Gap (Floating)
# Move inst2 slightly higher
$p set $x1 $y1 [expr $z1 + $t1 + 1]
$inst2 setLoc $p

check_3dblox
set category [$top_chip findMarkerCategory "3DBlox"]
set floating_category [$category findMarkerCategory "Floating chips"]
check "Vertical gap detected as floating" { expr { $floating_category != "NULL" && [$floating_category getMarkerCount] > 0 } } 1

# 5. Test Multiple Floating Sets
# Create another chip far away
set inst3 [odb::dbChipInst_create $top_chip $master1 "inst3"]
$p set [expr $x1 + 10 * $w1] [expr $y1 + 10 * $h1] $z1
$inst3 setLoc $p

check_3dblox
set category [$top_chip findMarkerCategory "3DBlox"]
set floating_category [$category findMarkerCategory "Floating chips"]
check "Multiple floating sets detected" { expr { $floating_category != "NULL" && [$floating_category getMarkerCount] >= 2 } } 1

exit_summary
