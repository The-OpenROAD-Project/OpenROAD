source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "multiple_calls.def"

set guide_file1 [make_result_file mc1_route.guide]
set guide_file2 [make_result_file mc2_route.guide]

fastroute -output_file $guide_file1 \
          -grid_origin {0 0}

FastRoute::clear_fastroute

set_global_routing_layer_adjustment * 0.8

fastroute -output_file $guide_file2

diff_file $guide_file1 $guide_file2
