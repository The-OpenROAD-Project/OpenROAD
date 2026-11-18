source "helpers.tcl"

set OR $argv0
set server1 [exec $OR server1.tcl > [make_result_file server1.log] &]
set server2 [exec $OR server2.tcl > [make_result_file server2.log] &]
set balancer [exec $OR balancer.tcl > [make_result_file balancer.log] &]
set base [exec $OR -exit gcd_nangate45.tcl > [make_result_file base.log] &]
exec sleep 3
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def gcd_nangate45_preroute.def
read_guides gcd_nangate45.route_guide

set_thread_count [expr [cpu_count] / 4]
detailed_route -output_drc [make_result_file gcd_nangate45_distributed.output.drc.rpt] \
  -output_maze [make_result_file gcd_nangate45_distributed.output.maze.log] \
  -verbose 1 \
  -distributed \
  -remote_host 127.0.0.1 \
  -remote_port 1234 \
  -cloud_size 2 \
  -shared_volume [make_result_dir]
exec kill $server1
exec kill $server2
exec kill $balancer
set def_file [make_result_file gcd_nangate45.def]
write_def $def_file
set running [file exists /proc/$base]
while { $running } {
  sleep 1
  set running [file exists /proc/$base]
}
diff_files [make_result_file gcd_nangate45.defok] $def_file
