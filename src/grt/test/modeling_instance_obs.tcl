# macro obstacles not aligned on different layers
source "helpers.tcl"
read_lef "macro_obs_not_aligned.lef"
read_def "modeling_instance_obs.def"

set guide_file [make_result_file modeling_instance_obs.guide]

set_routing_layers -signal met1-met5

global_route -verbose

write_guides $guide_file

diff_file modeling_instance_obs.guideok $guide_file
