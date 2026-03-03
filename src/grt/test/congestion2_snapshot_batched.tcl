source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set_thread_count 16

set guide_file [make_result_file congestion2_snapshot_batched.guide]

set_global_routing_layer_adjustment metal2 0.9
set_global_routing_layer_adjustment metal3 0.9
set_global_routing_layer_adjustment metal4-metal6 0.9
set_global_routing_layer_adjustment metal7-metal10 1.0

set_routing_layers -signal metal2-metal10

global_route -allow_congestion -snapshot_batched_width 16 -verbose

write_guides $guide_file

diff_file congestion2_snapshot_batched.guideok $guide_file
