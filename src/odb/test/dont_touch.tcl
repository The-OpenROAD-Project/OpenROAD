source "helpers.tcl"


set db [ord::get_db]
read_lef "data/gscl45nm.lef"
read_def "data/design.def"
set chip [$db getChip]
set block [$chip getBlock]

set nets [$block getNets]
set net [lindex $nets 0]
set wire [odb::dbWire_create $net]

$net setDoNotTouch true
catch {odb::dbNet_destroy $net}
catch {odb::dbWire_create $net}
catch {odb::dbWire_destroy $wire}
catch {$net destroySWires}

set insts [$block getInsts]
set inst [lindex $insts 0]

$inst setDoNotTouch true
catch {odb::dbInst_destroy $inst}
catch {$inst setOrigin 1 1}
catch {$inst setOrient R180}
catch {$inst setPlacementStatus FIRM}

puts "pass"
exit
