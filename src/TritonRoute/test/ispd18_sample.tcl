source "helpers.tcl"

read_lef testcase/ispd18_sample/ispd18_sample.input.lef
read_def testcase/ispd18_sample/ispd18_sample.input.def
detailed_route -guide testcase/ispd18_sample/ispd18_sample.input.guide \
               -outputguide results/ispd18_sample.output_guide.mod \
               -outputDRC results/ispd18_sample.output_DRC.rpt \
               -outputmaze results/ispd18_sample.output_Maze.log \
               -verbose 0

set def_file [make_result_file ispd18_sample.def]
write_def $def_file
diff_files ispd18_sample.defok $def_file
