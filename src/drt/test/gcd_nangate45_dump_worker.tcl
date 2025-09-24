source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def gcd_nangate45_preroute.def
read_guides gcd_nangate45.route_guide
set_thread_count [cpu_count]

detailed_route_debug -dump_dr -dump_dir [make_result_dir] -iter 2

detailed_route -output_guide [make_result_file gcd_nangate45.output.guide.mod] \
  -output_drc [make_result_file gcd_nangate45.output.drc.rpt] \
  -output_maze [make_result_file gcd_nangate45.output.maze.log] \
  -verbose 1
set def_file [make_result_file gcd_nangate45.defok]
write_def $def_file
