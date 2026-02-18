source "helpers.tcl"

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
check "Initial overlap count" { get_3dblox_marker_count "Overlapping chips" } 0
check "Initial floating count" { get_3dblox_marker_count "Floating chips" } 0

# 2. Test Touching (Stacked Exactly)
# Place inst2 exactly on top of inst1
set p [odb::Point3D]
$p set $x1 $y1 [expr $z1 + $t1]
$inst2 setLoc $p

check_3dblox
check "Touching chips not floating" { get_3dblox_marker_count "Floating chips" } 0

# 3. Test Vertical Gap (Floating)
# Move inst2 significantly higher (more than connection thickness of 4000)
$p set $x1 $y1 [expr $z1 + $t1 + 10000]
$inst2 setLoc $p

check_3dblox
check "Vertical gap detected as floating" { get_3dblox_marker_count "Floating chips" } 1

# 4. Test Multiple Floating Sets
# Create another chip far away
set inst3 [odb::dbChipInst_create $top_chip $master1 "inst3"]
$p set [expr $x1 + 10 * $w1] [expr $y1 + 10 * $h1] $z1
$inst3 setLoc $p

check_3dblox
# Should find 2 sets of floating chips (inst1, inst2 and inst3 are all separate from each other)
check "Multiple floating sets detected" { get_3dblox_marker_count "Floating chips" } 2

# 5. Test Partial Overlap
# Move inst2 to partially overlap with inst1
set p [odb::Point3D]
$p set [expr $x1 + $w1 / 4] [expr $y1 + $h1 / 4] [expr $z1 + $t1 / 2]
$inst2 setLoc $p

check_3dblox
check "Partial overlap detected" { get_3dblox_marker_count "Overlapping chips" } 1

exit_summary
