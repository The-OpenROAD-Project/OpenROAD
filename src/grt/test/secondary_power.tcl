# Test routing secondary power net from gate pin to the nearby pre-routed strap
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "secondary_power.def"

# Mark the net as a non-special supply net (secondary power net)
set db [ord::get_db]
set chip [$db getChip]
set block [$chip getBlock]
set net [$block findNet "VDDD"]
$net clearSpecial

set guide_file [make_result_file secondary_power.guide]

set_routing_layers -signal metal1-metal4

global_route -verbose

write_guides $guide_file

diff_file secondary_power.guideok $guide_file
