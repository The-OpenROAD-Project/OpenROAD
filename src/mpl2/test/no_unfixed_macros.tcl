# Case in which there are no unfixed macros in the input .def
# so the macro placement should just be skipped.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45_tech.lef"
read_lef "./testcases/macro_only.lef"
read_liberty "./testcases/macro_only.lib"

read_verilog "./testcases/macro_only.v"
link_design "macro_only"

read_def "./testcases/no_unfixed_macros.def" -floorplan_initialize

set_thread_count 0
rtl_macro_placer -report_directory results/no_unfixed_macros -halo_width 4.0

set def_file [make_result_file no_unfixed_macros.def]
write_def $def_file

diff_files no_unfixed_macros.defok $def_file