read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def aes_nangate45_preroute.def
set_thread_count [expr [exec getconf _NPROCESSORS_ONLN] / 4]
detailed_route -guide aes_nangate45.route_guide \
               -output_guide results/aes_nangate45.output.guide.mod \
               -output_drc results/aes_nangate45.output.drc.rpt \
               -output_maze results/aes_nangate45.output.maze.log \
               -verbose 1 
set def_file results/aes_nangate45.defok
write_def $def_file
