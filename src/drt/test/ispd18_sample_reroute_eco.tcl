# Reroute from DRC markers when some nets have no routing (an ECO, e.g. after
# repair_design or repair_antennas rip up the nets they touch): the ripped-up
# nets must be routed, not skipped by resuming past the initial iterations.
source "helpers.tcl"

read_lef testcase/ispd18_sample/ispd18_sample.input.lef
read_def ispd18_sample_reroute.input.def
read_guides testcase/ispd18_sample/ispd18_sample.input.guide
set_routing_layers -signal Metal1-Metal9
odb::dbMarkerCategory_fromTR [[ord::get_db] getChip] DRC \
  ispd18_sample_reroute.drc.rpt

set block [ord::get_db_block]
foreach name {net1233 net1236} {
  odb::dbWire_destroy [[$block findNet $name] getWire]
}
puts "unrouted before: [expr {![design_is_routed]}]"

detailed_route -output_drc [make_result_file ispd18_sample_reroute_eco.drc.rpt] \
  -verbose 0

if { ![design_is_routed -verbose] } {
  error "ripped-up nets were not routed"
}
puts "unrouted after: 0"
