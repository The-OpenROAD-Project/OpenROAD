# Case in which we verify macros with CLOCK pins can
# be placed.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45_tech.lef"
read_lef "./testcases/clocked_macro.lef"
read_liberty "./testcases/clocked_macro.lib"

read_verilog "./testcases/clocked_macro.v"
link_design "clocked_macro"

read_def "./testcases/clocked_macro.def" -floorplan_initialize

set_thread_count 0
rtl_macro_placer -report_directory results/clocked_macro -halo_width 4.0

set def_file [make_result_file clocked_macro.def]
write_def $def_file

diff_files clocked_macro.defok $def_file