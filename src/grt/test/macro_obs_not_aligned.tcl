# macro obstacles not aligned on different layers
source "helpers.tcl"
read_lef "macro_obs_not_aligned.lef"
read_def "macro_obs_not_aligned.def"

set guide_file [make_result_file macro_obs_not_aligned.guide]

set_global_routing_layer_adjustment met1-met5 0.7

set_routing_layers -signal met1-met5

global_route -verbose

write_guides $guide_file

diff_file macro_obs_not_aligned.guideok $guide_file
