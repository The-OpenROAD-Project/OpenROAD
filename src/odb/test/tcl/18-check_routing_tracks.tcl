set tcl_dir [file dirname [file normalize [info script]]]
set tests_dir [file dirname $tcl_dir]
set data_dir [file join $tests_dir "data"]
set opendb_dir [file dirname $tests_dir]
source [file join $tcl_dir "test_helpers.tcl"]

set db [odb::dbDatabase_create]
set lib [odb::read_lef $db [file join $data_dir "Nangate45" "NangateOpenCellLibrary.mod.lef"]]
odb::read_def $db [file join $data_dir "gcd" "floorplan.def"]
set chip [$db getChip]
set tech [$db getTech]

set block [$chip getBlock]

set routing_layers_count [$tech getRoutingLayerCount]

for {set l 1} {$l <= $routing_layers_count} {incr l} {
    set routingLayer [$tech findRoutingLayer $l]

    if {$routingLayer == "NULL"} {
        puts "Read routing layer Failed"
        exit 1
    }

    set routingTrack [$block findTrackGrid $routingLayer]
    if {$routingTrack == "NULL"} {
        puts "Read routing track for layer $l Failed"
        exit 1
    }
}
