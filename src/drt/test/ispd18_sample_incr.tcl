source "helpers.tcl"

read_lef testcase/ispd18_sample/ispd18_sample.input.lef
read_def testcase/ispd18_sample/ispd18_sample.input.def
read_guides testcase/ispd18_sample/ispd18_sample.input.guide
detailed_route -output_drc results/ispd18_sample.output.drc.rpt \
               -output_maze results/ispd18_sample.output.maze.log \
               -output_guide_coverage results/ispd18_sample.coverage.csv \
               -verbose 0
set net [[ord::get_db_block] findNet "net1231"]
odb::dbWire_destroy [$net getWire]
detailed_route -output_drc results/ispd18_sample.output.drc.rpt \
               -output_maze results/ispd18_sample.output.maze.log \
               -output_guide_coverage results/ispd18_sample.coverage.csv \
               -verbose 0
set def_file [make_result_file ispd18_sample.def]
write_def $def_file
diff_files ispd18_sample.defok $def_file
