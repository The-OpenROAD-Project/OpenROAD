source "helpers.tcl"

set db [ord::get_db]
read_lef "data/Nangate45/NangateOpenCellLibrary.mod.lef"
read_def "data/gcd/floorplan.def"
set chip [$db getChip]
set tech [$db getTech]

set block [$chip getBlock]

set routing_layers_count [$tech getRoutingLayerCount]

for {set l 1} {$l <= $routing_layers_count} {incr l} {
    set routingLayer [$tech findRoutingLayer $l]

    if {$routingLayer == "NULL"} {
        puts "FAIL: Read routing layer Failed"
        exit 1
    }

    set routingTrack [$block findTrackGrid $routingLayer]
    if {$routingTrack == "NULL"} {
        puts "FAIL: Read routing track for layer $l Failed"
        exit 1
    }
}

puts "pass"
exit 0