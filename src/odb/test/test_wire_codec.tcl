#package require control
#control::control assert enabled 1
#
# Converted from test_wire_codec.py
#
source "helper.tcl"

# Set up data
lassign [createMultiLayerDB] db tech m1 m2 m3 v12 v23
set chip [odb::dbChip_create $db]
set block [odb::dbBlock_create $chip "chip"]
set net [odb::dbNet_create $block "net"]
set wire [odb::dbWire_create $net]

set encoder [odb::dbWireEncoder]
$encoder begin $wire
$encoder newPath $m1 "ROUTED"
$encoder addPoint 2000 2000
set j1 [$encoder addPoint 10000 2000]
$encoder addPoint 18000 2000
$encoder newPath $j1
$encoder addTechVia $v12
set j2 [$encoder addPoint 10000 10000]
$encoder addPoint 10000 18000
$encoder newPath $j2
set j3 [$encoder addTechVia $v12]
$encoder addPoint 23000 10000 4000
$encoder newPath $j3
$encoder addPoint 3000 10000
$encoder addTechVia $v12
$encoder addTechVia $v23
$encoder addPoint 3000 10000 4000
$encoder addPoint 3000 18000 6000
$encoder end

# Start decoding process
set decoder [odb::dbWireDecoder]
$decoder begin $wire

# Encoding started with a path
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_PATH} [format "nextop isn't path: %d" $nextOp]

# Check first point
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_POINT} [format "nextop isn't point: %d" $nextOp]
set point [$decoder getPoint]
assert {[lequal $point [list 2000 2000]]} "point list doesn't match"

# Check second point
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_POINT} [format "nextop isn't point: %d" $nextOp]
set point [$decoder getPoint]
assert {[lequal $point [list 10000 2000]]} "point list doesn't match"

# Check third point
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_POINT} [format  "nextop isn't point: %d" $nextOp]
set point [$decoder getPoint]
assert {[lequal $point [list 18000 2000]]} "point list doesn't match"

# Check first junction id
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_JUNCTION} [format   "nextop isn't jun: %d" $nextOp]
set jid [$decoder getJunctionValue]
assert {$jid == $j1} "jun value doesn't match"

# Check junction point
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_POINT} [format  "nextop isn't point: %d" $nextOp]
set point [$decoder getPoint]
assert {[lequal $point [list 10000 2000]]} "point list doesn't match"

# Check tech via
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_TECH_VIA} [format  "nextop isn't via: %d" $nextOp]
set tchVia [$decoder getTechVia]
assertStringEq [$tchVia getName] [$v12 getName] [format "techvia name doesn't match: %s %s" [$tchVia getName] [$v12 getName]]

# Check next point
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_POINT} [format  "nextop isn't point: %d" $nextOp]
set point [$decoder getPoint]
assert {[lequal $point [list 10000 10000]]} "point list doesn't match"

# Check next point
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_POINT} [format  "nextop isn't point: %d" $nextOp]
set point [$decoder getPoint]
assert {[lequal $point [list 10000 18000]]} "point list doesn't match"

# Check second junction id
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_JUNCTION} [format   "nextop isn't jun: %d" $nextOp]
set jid [$decoder getJunctionValue]
assert {$jid == $j2} "jun value doesn't match"

# Check junction point
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_POINT} [format  "nextop isn't point: %d" $nextOp]
set point [$decoder getPoint]
assert {[lequal $point [list 10000 10000]]} "point list doesn't match"

# Check tech via
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_TECH_VIA} [format  "nextop isn't via: %d" $nextOp]
set tchVia [$decoder getTechVia]
assertStringEq [$tchVia getName] [$v12 getName] [format "techvia name doesn't match: %s %s" [$tchVia getName] [$v12 getName]]

# Check next point
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_POINT_EXT} [format "nextop isn't point_ext: %d" $nextOp]
set point [$decoder getPoint_ext]
assert {[lequal $point [list 23000 10000 4000]]} "point list doesn't match"

# Check third junction id
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_JUNCTION} [format "nextop isn't jun: %d" $nextOp]
set jid [$decoder getJunctionValue]
assert {$jid == $j3} "jun value doesn't match"

# Check junction point
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_POINT} [format "nextop isn't point: %d" $nextOp]
set point [$decoder getPoint]
assert {[lequal $point [list 10000 10000]]} "point list doesn't match"

# Check next point
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_POINT} [format "nextop isn't point: %d" $nextOp]
set point [$decoder getPoint]
assert {[lequal $point [list 3000 10000]]} "point list doesn't match"

# Check tech via
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_TECH_VIA} [format "nextop isn't via: %d" $nextOp]
set tchVia [$decoder getTechVia]
assertStringEq [$tchVia getName] [$v12 getName] [format "techvia name doesn't match: %s %s" [$tchVia getName] [$v12 getName]]

# Check tech via
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_TECH_VIA} [format "nextop isn't via: %d" $nextOp]
set tchVia [$decoder getTechVia]
assertStringEq [$tchVia getName] [$v23 getName] [format "techvia name doesn't match: %s %s" [$tchVia getName] [$v23 getName]]

# Check next point
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_POINT_EXT} [format "nextop isn't point_ext: %d" $nextOp]
set point [$decoder getPoint_ext]
assert {[lequal $point [list 3000 10000 4000]]} "point list doesn't match"

# Check next point
set nextOp [$decoder next]
assert {$nextOp == $odb::dbWireDecoder_POINT_EXT} [format "nextop isn't point_ext: %d" $nextOp]
set point [$decoder getPoint_ext]
assert {[lequal $point [list 3000 18000 6000]]} "point list doesn't match"

puts "pass"
exit 0
