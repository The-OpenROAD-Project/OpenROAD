source "helpers.tcl"

set OR $argv0
set server1 [exec $OR server1.tcl > results/server1.log &]
set server2 [exec $OR server2.tcl > results/server2.log &]
set balancer [exec $OR balancer.tcl > results/balancer.log &]
set base [exec $OR -exit aes_nangate45.tcl > results/base.log &]

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def aes_nangate45_preroute.def
read_guides aes_nangate45.route_guide
set_thread_count [expr [exec getconf _NPROCESSORS_ONLN] / 4]
detailed_route -output_drc results/aes_nangate45_distributed.output.drc.rpt \
               -output_maze results/aes_nangate45_distributed.output.maze.log \
               -verbose 1 \
	             -distributed \
	             -remote_host 127.0.0.1 \
	             -remote_port 1234 \
                 -cloud_size 2 \
               -shared_volume results 
exec kill $server1
exec kill $server2
exec kill $balancer
set def_file results/aes_nangate45.def
write_def $def_file
set running [file exists /proc/$base]
while { $running } {
  exec sleep 1
  set running [file exists /proc/$base]
}
diff_files results/aes_nangate45.defok $def_file
