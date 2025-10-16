# check route guides for gcd_nangate45. def file from the openroad-flow
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set guide_file [make_result_file cugr_adjustment1.guide]

set_global_routing_layer_adjustment metal1 0.8
set_global_routing_layer_adjustment metal2 0.7
set_global_routing_layer_adjustment * 0.5

global_route -verbose -use_cugr

write_guides $guide_file

diff_file cugr_adjustment1.guideok $guide_file
