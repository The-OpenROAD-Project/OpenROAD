# Case in which we treat the root as a macro cluster and jump
# from initializing the physical tree to macro placement
source "helpers.tcl"

read_lef "./Nangate45/Nangate45_tech.lef"
read_lef "./testcases/macro_only.lef"
read_liberty "./testcases/macro_only.lib"

read_verilog "./testcases/macro_only.v"
link_design "macro_only"

read_def "./testcases/macro_only.def" -floorplan_initialize

set_thread_count 0
rtl_macro_placer -report_directory results/macro_only -halo_width 4.0

set def_file [make_result_file macro_only.def]
write_def $def_file

diff_files macro_only.defok $def_file