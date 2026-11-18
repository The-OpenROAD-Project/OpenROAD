# Resume from DRC markers in the db must still route nets left unrouted
# by an ECO (issue #11993).
source "helpers.tcl"

read_lef testcase/ispd18_sample/ispd18_sample.input.lef
read_def ispd18_sample_reroute.input.def
read_guides testcase/ispd18_sample/ispd18_sample.input.guide
set_routing_layers -signal Metal1-Metal9
odb::dbMarkerCategory_fromTR [[ord::get_db] getChip] DRC \
  ispd18_sample_reroute.drc.rpt

set block [ord::get_db_block]
odb::dbWire_destroy [[$block findNet "net1233"] getWire]
puts "routed before: [design_is_routed]"

detailed_route -verbose 0
puts "routed after: [design_is_routed -verbose]"
