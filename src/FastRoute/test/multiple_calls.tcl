source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "multiple_calls.def"

set guide_file1 [make_result_file mc1_route.guide]
set guide_file2 [make_result_file mc2_route.guide]

fastroute -grid_origin {0 0}
write_guides $guide_file1

FastRoute::clear_fastroute

fastroute -capacity_adjustment 0.8
write_guides $guide_file2

diff_file $guide_file1 $guide_file2
