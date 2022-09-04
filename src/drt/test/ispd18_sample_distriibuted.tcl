source "helpers.tcl"

read_lef testcase/ispd18_sample/ispd18_sample.input.lef
read_def testcase/ispd18_sample/ispd18_sample.input.def
run_worker -host 127.0.0.1 -port 2222 -i
detailed_route -guide testcase/ispd18_sample/ispd18_sample.input.guide \
               -output_guide results/ispd18_sample.output.guide.mod \
               -output_drc results/ispd18_sample.output.drc.rpt \
               -output_maze results/ispd18_sample.output.maze.log \
               -verbose 1 \
	             -distributed \
	             -remote_host 127.0.0.1 \
	             -remote_port 1111 \
               -shared_volume results

set def_file [make_result_file ispd18_sample.def]
write_def $def_file
diff_files ispd18_sample.defok $def_file
