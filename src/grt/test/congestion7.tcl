# check route guides for gcd_nangate45. def file from the openroad-flow
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set guide_file [make_result_file congestion7.guide]
set rpt_file [make_result_file congestion7.rpt]

set_global_routing_layer_adjustment metal2 0.9
set_global_routing_layer_adjustment metal3 0.9
set_global_routing_layer_adjustment metal4-metal10 1

set_routing_layers -signal metal2-metal10

global_route -allow_congestion -verbose -congestion_report_file $rpt_file

write_guides $guide_file

diff_file congestion7.guideok $guide_file
diff_file congestion7.rptok $rpt_file
