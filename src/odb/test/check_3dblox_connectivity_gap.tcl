source "helpers.tcl"

# Create a design with stacked chips but a large thickness and gap
# This tests if the connection 'thickness' (bloat) is correctly handled.

read_3dbx "data/example_hard.3dbx"
set top_chip [[ord::get_db] getChip]

set inst1 [$top_chip findChipInst "soc_inst"]
set t1 [[$inst1 getMasterChip] getThickness]
set c_thick 0
foreach conn [$top_chip getChipConns] {
  if { [$conn getName] == "soc_to_soc" } {
    set c_thick [$conn getThickness]
  }
}

# Set location to exactly match connection thickness
set inst2 [$top_chip findChipInst "soc_inst_duplicate"]
set p [odb::Point3D]
lassign [$inst2 getLoc] x2 y2 z2
$p set $x2 $y2 [expr $t1 + $c_thick]
$inst2 setLoc $p

check_3dblox
check "Hard 1: Connection with gap (intersecting)" { get_3dblox_marker_count "Floating chips" } 0

# Now move it further to break connection
# Gap must be > 2 * c_thick.
# Gap = z - t1 > 2 * c_thick => z > t1 + 2 * c_thick
set u [[ord::get_db] getDbuPerMicron]
lassign [$inst2 getLoc] x2 y2 z2
$p set $x2 $y2 [expr $t1 + 2 * $c_thick + 10 * $u]
$inst2 setLoc $p

check_3dblox
check "Hard 1: Connection with gap (broken)" { get_3dblox_marker_count "Floating chips" } 1

# puts "pass"
exit_summary
