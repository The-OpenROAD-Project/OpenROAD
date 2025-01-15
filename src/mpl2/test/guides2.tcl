# This test case places both long macros at the right
# side, while the normal result have one at each side
# A guidance weight of 30 had to be used to outweight 
# the wirelength penalty
source "helpers.tcl"

read_lef "./Nangate45/Nangate45_tech.lef"
read_lef "./testcases/macro_only.lef"
read_liberty "./testcases/macro_only.lib"

read_verilog "./testcases/macro_only.v"
link_design "macro_only"

read_def "./testcases/macro_only.def" -floorplan_initialize

set_macro_guidance_region -macro_name U6 -region {220 5 328 442}
set_macro_guidance_region -macro_name U1 -region {328 5 436 442}

set_thread_count 0
rtl_macro_placer -report_directory results/guides2 -halo_width 4.0 -guidance_weight 30.0

set def_file [make_result_file guides2.def]
write_def $def_file

diff_files guides2.defok $def_file