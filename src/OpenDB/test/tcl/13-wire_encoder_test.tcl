set tcl_dir [file dirname [file normalize [info script]]]
set tests_dir [file dirname $tcl_dir]
set data_dir [file join $tests_dir "data"]
set opendb_dir [file dirname $tests_dir]
source [file join $tcl_dir "test_helpers.tcl"]

set db [odb::dbDatabase_create]
set lib [odb::read_lef $db [file join $data_dir "gscl45nm.lef"]]
set tech [$db getTech]
odb::read_def $db [file join $data_dir "design.def"]
set chip [$db getChip]

set vias [$tech getVias]
set via1 [lindex $vias 0]
set layer1 [$via1 getBottomLayer]
set via2 [lindex $vias 1]
set via3 [lindex $vias 2]

set block [$chip getBlock]
set net [odb::dbNet_create $block "w1"]
set wire [odb::dbWire_create $net]
set wire_encoder [odb::dbWireEncoder]

# Encoding
$wire_encoder begin $wire
# void newPath( dbTechLayer * layer, dbWireType type )
$wire_encoder newPath $layer1 "ROUTED" 
# int addPoint( int x, int y, uint property = 0 );
$wire_encoder addPoint 2000 2000
set jid1 [$wire_encoder addPoint 10000 2000]
$wire_encoder addPoint 18000 2000
$wire_encoder newPath $jid1
# int addTechVia( dbTechVia * via );
$wire_encoder addTechVia $via1
set jid2 [$wire_encoder addPoint 10000 10000]
$wire_encoder addPoint 10000 18000
$wire_encoder newPath $jid2
set jid3 [$wire_encoder addTechVia $via2]
$wire_encoder end

set result [odb::write_def $block $opendb_dir/build/wire_encoder.def]
exit [expr $result != 1]
