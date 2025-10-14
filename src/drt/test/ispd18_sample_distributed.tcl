source "helpers.tcl"
set OR $argv0
set server1 [exec $OR server1.tcl > [make_result_file server1.log] &]
set server2 [exec $OR server2.tcl > [make_result_file server2.log] &]
set balancer [exec $OR balancer.tcl > [make_result_file balancer.log] &]
exec sleep 3

read_lef testcase/ispd18_sample/ispd18_sample.input.lef
read_def testcase/ispd18_sample/ispd18_sample.input.def
read_guides testcase/ispd18_sample/ispd18_sample.input.guide
detailed_route -output_drc [make_result_file ispd18_sample.output.drc.rpt] \
  -output_maze [make_result_file ispd18_sample.output.maze.log] \
  -verbose 1 \
  -distributed \
  -remote_host 127.0.0.1 \
  -remote_port 1234 \
  -cloud_size 2 \
  -shared_volume [make_result_dir]
exec kill $server1
exec kill $server2
exec kill $balancer

set def_file [make_result_file ispd18_sample.def]
write_def $def_file
diff_files ispd18_sample.defok $def_file
