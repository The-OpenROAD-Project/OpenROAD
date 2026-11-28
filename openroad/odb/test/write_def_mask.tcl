source "helpers.tcl"

set db [ord::get_db]
read_lef "data/write_def_mask.lef"
read_def "data/write_def_mask.def"

set block [ord::get_db_block]

# Net b: (50 150) MASK 1 (500 150) (950 150)
# Second point has mask 1, third point has no mask
set net_b [$block findNet "b"]
set wire_b [$net_b getWire]
set decoder [odb::dbWireDecoder]
$decoder begin $wire_b
$decoder next ;# PATH
$decoder next ;# (50 150)
check "net b pt1 color is null" {$decoder getColor} NULL
$decoder next ;# (500 150)
check "net b pt2 color is 1" {$decoder getColor} 1
$decoder next ;# (950 150)
check "net b pt3 color is null" {$decoder getColor} NULL

# Net c: NEW path after masked segment
# Path 1 pt2 has mask 1, Path 2 pt2 has no mask
set net_c [$block findNet "c"]
set wire_c [$net_c getWire]
$decoder begin $wire_c
$decoder next ;# PATH
$decoder next ;# (50 250)
$decoder next ;# (500 250)
check "net c path 1 pt2 color is 1" {$decoder getColor} 1
$decoder next ;# PATH (NEW)
$decoder next ;# (50 350)
$decoder next ;# (500 350)
check "net c path 2 pt2 color is null" {$decoder getColor} NULL

set out_def [make_result_file "write_def_mask.def"]
write_def $out_def

diff_files $out_def "write_def_mask.defok"

puts "pass"
exit 0
