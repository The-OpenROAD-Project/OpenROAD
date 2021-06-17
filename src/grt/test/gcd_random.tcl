# check route guides for gcd_nangate45. def file from the openroad-flow
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set guide_file [make_result_file gcd_random.guide]

set_global_routing_random -seed 1

global_route

write_guides $guide_file

diff_file gcd_random.guideok $guide_file
