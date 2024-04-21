source "helpers.tcl"

read_lef testcase/ispd18_sample/ispd18_sample.input.lef
read_def ispd18_sample.defok
read_guides testcase/ispd18_sample/ispd18_sample.input.guide

set net [[ord::get_db_block] findNet "net1231"]
odb::dbWire_destroy [$net getWire]

detailed_route -output_drc results/single_step.output.drc.rpt \
               -output_maze results/single_step.output.maze.log \
               -verbose 0 \
               -single_step_dr
drt::step_dr 7  0 3 8 0 8 0.95 5 true
drt::step_end

set def_file [make_result_file single_step_incr.def]
write_def $def_file
diff_files single_step_incr.defok $def_file
