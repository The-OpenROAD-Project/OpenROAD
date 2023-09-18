source "helpers.tcl"

set db [ord::get_db]
read_lef "data/sky130hd/sky130hd_multi_patterned.tlef"
read_def "data/sky130hd_multi_patterned.def"

set block [ord::get_db_block]
set colored_net [$block findNet "DU6_M10uuM11_W38W38_S00000S00000_2"]
set colored_wire [$colored_net getWire]
set wire_decoder [odb::dbWireDecoder]
$wire_decoder begin $colored_wire

# routed
$wire_decoder next
# first point
$wire_decoder next
check "color is null" {$wire_decoder getColor} NULL
# second point. Color appears between two points.
$wire_decoder next
check "color is not null" {$wire_decoder getColor} 1



puts "pass"
exit 0
