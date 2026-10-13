# set_wire_rc -redistribution_layer with RDL chips on different technologies
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def reg3.def

read_lef -tech -tech_name tech2 Nangate45/Nangate45.lef
set db [ord::get_db]
odb::dbChip_create $db [$db findTech tech2] "rdl_a" "RDL"
odb::dbChip_create $db [$db findTech Nangate45] "rdl_b" "RDL"

# explicit values apply to both RDL chips
set_wire_rc -redistribution_layer -resistance 2e-3 -capacitance 2e-1
# a shared layer cannot be resolved across different technologies
catch { set_wire_rc -redistribution_layer -layer metal3 } msg
puts $msg
