source "helpers.tcl"

# set OR $argv0
# set server1 [$OR server1.tcl > results/server1.log &]
# set server2 [$OR server2.tcl > results/server2.log &]
# set balancer [$OR balancer.tcl > results/balancer.log &]
# set base [$OR aes_nangate45.tcl > results/base.log &]

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def aes_nangate45_preroute.def
set_thread_count [exec getconf _NPROCESSORS_ONLN]
run_worker -host 127.0.0.1 -port 2222 -i
detailed_route -guide aes_nangate45.route_guide \
               -output_guide results/aes_nangate45_distributed.output.guide.mod \
               -output_drc results/aes_nangate45_distributed.output.drc.rpt \
               -output_maze results/aes_nangate45_distributed.output.maze.log \
               -verbose 1 \
	             -distributed \
	             -remote_host 127.0.0.1 \
	             -remote_port 1111 \
               -shared_volume results
# exec kill $server1
# exec kill $server2
# exec kill $balancer
set def_file results/aes_nangate45.def
write_def $def_file
diff_files results/aes_nangate45.defok $def_file
