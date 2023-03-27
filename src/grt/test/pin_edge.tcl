source "helpers.tcl"
read_lef pin_edge.lef  
read_def pin_edge.def

set guide_file [make_result_file pin_edge.guide]

global_route

write_guides $guide_file

diff_file pin_edge.guideok $guide_file
