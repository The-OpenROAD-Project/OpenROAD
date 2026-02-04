source "helpers.tcl"

# Setup database and tech
set db [ord::get_db]
set tech [odb::dbTech_create $db "tech"]
set top_chip [odb::dbChip_create $db $tech "TopChip" "HIER"]
$top_chip setWidth 10000
$top_chip setHeight 10000
$top_chip setThickness 1000

# Create master chips
set chip1 [odb::dbChip_create $db $tech "Chip1" "DIE"]
$chip1 setWidth 2000
$chip1 setHeight 2000
$chip1 setThickness 500

# Create a main chip instance
set inst1 [odb::dbChipInst_create $top_chip $chip1 "inst1"]
set p1 [odb::Point3D]
$p1 set 0 0 0
$inst1 setLoc $p1

# Create a floating chip instance (far away from inst1)
set inst2 [odb::dbChipInst_create $top_chip $chip1 "inst2"]
set p2 [odb::Point3D]
$p2 set 5000 5000 0
$inst2 setLoc $p2

# Run checker
check_3dblox

# Verify markers
set marker_count [get_3dblox_marker_count "Floating chips"]
if { $marker_count == 0 } {
  puts "FAIL: No floating markers found"
  exit 1
}

puts "pass"
exit
