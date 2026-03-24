# Test if we take into account correctly macros that don't have pins and
# that are inside the macro placement area along with fixed macros.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./Nangate45_io/dummy_pads.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/macros_without_pins1.def"

set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir] -halo_width 0.3

set def_file [make_result_file macros_without_pins1.def]
write_def $def_file

diff_files macros_without_pins1.defok $def_file
