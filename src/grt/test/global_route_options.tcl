# global_route -options gives the same routes as multiple_calls' switches.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "multiple_calls.def"

set guide_file1 [make_result_file gro1_route.guide]
set guide_file2 [make_result_file gro2_route.guide]

global_route -options {grid_origin {0 0} verbose true}
write_guides $guide_file1

set_global_routing_layer_adjustment * 0.8
global_route -options {verbose true}
write_guides $guide_file2

diff_files multiple_calls.guideok1 $guide_file1
diff_files multiple_calls.guideok2 $guide_file2
