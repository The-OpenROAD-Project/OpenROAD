# report_congestion and has_congestion with CUGR, which can end an incremental
# global routing with congestion.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

# Tight capacity so congestion remains after every route.
set_global_routing_layer_adjustment metal1-metal10 0.9

global_route -use_cugr

global_route -start_incremental
# Moving an instance makes its nets dirty for the incremental reroute.
[[ord::get_db_block] findInst _441_] setLocation 60000 60000
global_route -end_incremental
puts "has_congestion: [has_congestion]"
if { [has_congestion] } {
  report_congestion
}
