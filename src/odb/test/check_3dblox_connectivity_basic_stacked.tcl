source "helpers.tcl"

# Load design
read_3dbx "data/example_fixed.3dbx"
set db [ord::get_db]
set top_chip [$db getChip]

# Get instances
set inst1 [$top_chip findChipInst "soc_inst"]
set inst2 [$top_chip findChipInst "soc_inst_duplicate"]

# Initially they overlap (stacked)
check_3dblox
check "Initial connection ok" { get_3dblox_marker_count "Connected regions" } 0

# Move inst2 far away so regions don't overlap
lassign [$inst1 getLoc] x1 y1 z1
set master1 [$inst1 getMasterChip]
set w1 [$master1 getWidth]
set h1 [$master1 getHeight]

set p [odb::Point3D]
$p set [expr $x1 + 10 * $w1] [expr $y1 + 10 * $h1] $z1
$inst2 setLoc $p

# Now check
check_3dblox
check "Connection mismatch detected" { get_3dblox_marker_count "Connected regions" } 1

exit_summary
