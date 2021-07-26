# test pd with alpha per min fanout
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set guide_file [make_result_file pd3.guide]

set_routing_alpha 0.9 -min_fanout 10

global_route

write_guides $guide_file

diff_file pd3.guideok $guide_file
