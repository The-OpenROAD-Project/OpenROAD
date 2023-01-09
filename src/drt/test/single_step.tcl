source "helpers.tcl"

read_lef testcase/ispd18_sample/ispd18_sample.input.lef
read_def testcase/ispd18_sample/ispd18_sample.input.def
read_guides testcase/ispd18_sample/ispd18_sample.input.guide

detailed_route -output_drc results/single_step.output.drc.rpt \
               -output_maze results/single_step.output.maze.log \
               -verbose 0 \
               -single_step_dr

drt::step_dr 7  0 3 8 0 8 0.95 1 true
drt::step_dr 7 -2 3 8 8 8 0.95 1 true
drt::step_dr 7 -5 3 8 8 8 0.95 1 true
drt::step_end

set def_file [make_result_file single_step.def]
write_def $def_file
# Single stepping should produce the same result as a regular run
diff_files ispd18_sample.defok $def_file
