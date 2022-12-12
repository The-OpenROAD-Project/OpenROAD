# test a design where a pin is not aligned with a routing track
source "helpers.tcl"
read_lef pin_track_not_aligned.lef
read_def pin_track_not_aligned.def

set guide_file [make_result_file pin_track_not_aligned.guide]

set_routing_layers -signal met1-met4

global_route

write_guides $guide_file

diff_file pin_track_not_aligned.guideok $guide_file

