read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def gcd_nangate45_preroute.def
set_thread_count [exec getconf _NPROCESSORS_ONLN]

detailed_route_debug -dump_dr -dump_dir results -worker " 90000 70000 " -iter 1 

detailed_route -guide gcd_nangate45.route_guide \
               -output_guide results/gcd_nangate45.output.guide.mod \
               -output_drc results/gcd_nangate45.output.drc.rpt \
               -output_maze results/gcd_nangate45.output.maze.log \
               -verbose 1 \
               -droute_end_iter 1
set def_file results/gcd_nangate45.defok
write_def $def_file
