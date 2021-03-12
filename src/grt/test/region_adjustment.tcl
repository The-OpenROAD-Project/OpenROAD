# region adjustment
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "region_adjustment.def"

set_global_routing_region_adjustment {1.4 2 20 15.5} -layer 2 -adjustment 0.9

set guide_file [make_result_file region_adjustment.guide]

global_route

write_guides $guide_file

diff_file region_adjustment.guideok $guide_file