# Test detailed routing secondary power net from gate pin to pre-routed strap
source "helpers.tcl"
read_lef "Nangate45/Nangate45_tech.lef"
read_lef "Nangate45/Nangate45_stdcell.lef"
read_def "secondary_power.def"

# Mark the net as a non-special supply net (secondary power net)
set db [ord::get_db]
set chip [$db getChip]
set block [$chip getBlock]
set net [$block findNet "VDDD"]
$net clearSpecial

set_routing_layers -signal metal1-metal4

global_route -verbose

detailed_route -output_drc [make_result_file secondary_power.output.drc.rpt] \
  -verbose 0

set def_file [make_result_file secondary_power.defok]
write_def $def_file
diff_files secondary_power.defok $def_file
