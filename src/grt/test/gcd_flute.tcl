# check route guides for gcd_nangate45. def file from the openroad-flow
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set guide_file [make_result_file gcd_flute.guide]

set_routing_alpha 0.0
global_route

write_guides $guide_file

diff_file gcd_flute.guideok $guide_file
