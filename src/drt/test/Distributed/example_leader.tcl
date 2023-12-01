
read_lef ../Nangate45/Nangate45_tech.lef
read_lef ../Nangate45/Nangate45_stdcell.lef
read_def ../gcd_nangate45_preroute.def
read_guides ../gcd_nangate45.route_guide

set_thread_count 16
detailed_route -output_drc ../results/gcd_nangate45_distributed.output.drc.rpt \
               -output_maze ../results/gcd_nangate45_distributed.output.maze.log \
               -verbose 1 \
	             -distributed \
	             -remote_host 127.0.0.1 \
	             -remote_port 1234 \
               -cloud_size 2 \
               -shared_volume ../results
