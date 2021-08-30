source "helpers.tcl"

read_lef testcase/ispd18_sample/ispd18_sample.input.lef
read_def testcase/ispd18_sample/ispd18_sample.input.def
detailed_route -guide testcase/ispd18_sample/ispd18_sample.input.guide \
               -output_guide results/ispd18_sample.output.guide.mod \
               -output_drc results/ispd18_sample.output.drc.rpt \
               -output_maze results/ispd18_sample.output.maze.log \
               -verbose 0

set def_file [make_result_file ispd18_sample.def]
write_def $def_file
diff_files ispd18_sample.defok $def_file
