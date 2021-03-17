# multiple calls for global_route
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "multiple_calls.def"

set guide_file1 [make_result_file mc1_route.guide]
set guide_file2 [make_result_file mc2_route.guide]

global_route -grid_origin {0 0}
write_guides $guide_file1

set_global_routing_layer_adjustment * 0.8

set_global_routing_layer_adjustment * 0.8
global_route
write_guides $guide_file2

diff_file $guide_file1 $guide_file2
