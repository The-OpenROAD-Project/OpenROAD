source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set guide_file1 [make_result_file mc1_route.guide]
set guide_file2 [make_result_file mc2_route.guide]

FastRoute::set_output_file $guide_file1
FastRoute::set_capacity_adjustment 0.0
FastRoute::set_min_layer 1
FastRoute::set_max_layer -1
FastRoute::set_verbose 0
FastRoute::set_grid_origin 0 0

FastRoute::start_fastroute
FastRoute::run_fastroute
FastRoute::write_guides

FastRoute::reset_fastroute

FastRoute::set_output_file $guide_file2
FastRoute::set_capacity_adjustment 0.15
FastRoute::set_min_layer 1
FastRoute::set_max_layer 10
FastRoute::set_verbose 0

FastRoute::start_fastroute
FastRoute::run_fastroute
FastRoute::write_guides

exit
