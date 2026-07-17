# set_wire_rc -chip/-tech/-redistribution_layer select chips in a multi-chip db
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def reg3.def

read_lef -tech -tech_name tech2 Nangate45/Nangate45.lef
set db [ord::get_db]
odb::dbChip_create $db [$db findTech tech2] "chip2" "RDL"

set_wire_rc -chip chip2 -resistance 1e-3 -capacitance 1e-1
set_wire_rc -tech tech2 -layer metal3
set_wire_rc -redistribution_layer -resistance 2e-3 -capacitance 2e-1
puts "selectors applied"
