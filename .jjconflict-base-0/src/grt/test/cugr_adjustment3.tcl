# check route guides for gcd_nangate45. def file from the openroad-flow
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set guide_file [make_result_file cugr_adjustment3.guide]

set_global_routing_layer_adjustment metal1-metal10 0.9

global_route -verbose -use_cugr

write_guides $guide_file

diff_file cugr_adjustment3.guideok $guide_file
