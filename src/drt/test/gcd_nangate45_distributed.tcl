source "helpers.tcl"

set OR $argv0
set server1 [$OR server1.tcl > results/server1.log &]
set server2 [$OR server2.tcl > results/server2.log &]
set balancer [$OR balancer.tcl > results/balancer.log &]
set base [$OR -exit gcd_nangate45.tcl > results/base.log &]

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def gcd_nangate45_preroute.def
set_thread_count 16
detailed_route -guide gcd_nangate45.route_guide \
               -output_guide results/gcd_nangate45_distributed.output.guide.mod \
               -output_drc results/gcd_nangate45_distributed.output.drc.rpt \
               -output_maze results/gcd_nangate45_distributed.output.maze.log \
               -verbose 1 \
	             -distributed \
	             -remote_host 127.0.0.1 \
	             -remote_port 1234 \
               -cloud_size 2 \
               -shared_volume results \
               -droute_end_iter 0
exec kill $server1
exec kill $server2
exec kill $balancer
set def_file results/gcd_nangate45.def
write_def $def_file
diff_files results/gcd_nangate45.defok $def_file
