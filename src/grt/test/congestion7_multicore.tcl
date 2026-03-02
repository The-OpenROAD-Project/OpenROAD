source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set_thread_count 4

set guide_file [make_result_file congestion7_multicore.guide]
set rpt_file [make_result_file congestion7_multicore.rpt]

set_global_routing_layer_adjustment metal2 0.9
set_global_routing_layer_adjustment metal3 0.9
set_global_routing_layer_adjustment metal4-metal10 1

set_routing_layers -signal metal2-metal10

global_route -allow_congestion -multicore -verbose -congestion_report_file $rpt_file \
  -congestion_report_iter_step 20

write_guides $guide_file

diff_file congestion7_multicore.guideok $guide_file
diff_file congestion7_multicore.rptok $rpt_file

set rpt_folder [file dirname $rpt_file]
diff_file congestion7_multicore-20.rptok \
  [file join $rpt_folder congestion7_multicore-tcl-20.rpt]
