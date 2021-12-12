source "helpers.tcl"

set OR $argv0
set server1 [$OR gcd_nangate45_distributed/server1.tcl > results/server1.log &]
set server2 [$OR gcd_nangate45_distributed/server2.tcl > results/server2.log &]
set balancer [$OR gcd_nangate45_distributed/balancer.tcl > results/balancer.log &]

read_lef gcd_nangate45_distributed/Nangate45_tech.lef
read_lef gcd_nangate45_distributed/Nangate45_stdcell.lef
read_def gcd_nangate45_distributed/gcd_nangate45_preroute.def
set_thread_count [exec getconf _NPROCESSORS_ONLN]
detailed_route -guide gcd_nangate45_distributed/gcd_nangate45.route_guide \
               -output_guide results/gcd_nangate45.output.guide.mod \
               -output_drc results/gcd_nangate45.output.drc.rpt \
               -output_maze results/gcd_nangate45.output.maze.log \
               -verbose 1 \
	             -distributed \
	             -remote_host 127.0.0.1 \
	             -remote_port 1234 \
               -shared_volume results
exec kill $server1
exec kill $server2
exec kill $balancer
set def_file results/gcd_nangate45.def
write_def $def_file
diff_files gcd_nangate45.defok $def_file
