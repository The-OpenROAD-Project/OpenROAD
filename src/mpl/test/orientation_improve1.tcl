# Test orientation improvement with one constrained pin.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/orientation_improve1.def"

# Run random PPL to incorporate the constraints into ODB
set_io_pin_constraint -direction INPUT -region right:10-30*
place_pins -annealing -random -hor_layers metal5 -ver_layer metal6

set_thread_count 0
rtl_macro_placer -report_directory results/orientation_improve1 -halo_width 0.3

set def_file [make_result_file orientation_improve1.def]
write_def $def_file

diff_files orientation_improve1.defok $def_file