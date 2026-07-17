# set_wire_rc -tech/-redistribution_layer target technologies in a multi-tech db
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def reg3.def

read_lef -tech -tech_name tech2 Nangate45/Nangate45.lef
set db [ord::get_db]
odb::dbChip_create $db [$db findTech tech2] "rdl" "RDL"

# a named technology, with layers resolved in that technology
set_wire_rc -tech tech2 -layer metal3
# every RDL chip's technology
set_wire_rc -redistribution_layer -resistance 2e-3 -capacitance 2e-1
puts "selectors applied"
