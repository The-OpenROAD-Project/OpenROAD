# Test if the guidance region is moving the macro
# to the left side (without a guide, the macro ends 
# up on the right side)
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"

read_lef "./testcases/macro_only.lef"
read_liberty "./testcases/macro_only.lib"

read_verilog "./testcases/guides1.v"
link_design "guides1"
read_def "./testcases/guides1.def" -floorplan_initialize

set_io_pin_constraint -direction INPUT -region left:*
place_pins -annealing -random -hor_layers metal5 -ver_layer metal6

set_macro_guidance_region -macro_name MACRO_1 -region {49 0 149 100}
set_thread_count 0
rtl_macro_placer -report_directory results/guides1 -halo_width 4.0

set def_file [make_result_file guides1.def]
write_def $def_file

diff_files guides1.defok $def_file