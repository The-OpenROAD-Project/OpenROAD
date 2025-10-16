# Test if both the pin access blockages and bundled nets are
# correctly generated for a block with a fixed IO pin and
# an unplaced constrained IO.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/macro_only.lef"
read_liberty "./Nangate45/Nangate45_fast.lib"
read_liberty "./testcases/macro_only.lib"

read_def "./testcases/mixed_ios1.def"

set_io_pin_constraint -pin_names {io_1} -region right:70-90

set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir] -halo_width 4.0

set def_file [make_result_file mixed_ios1.def]
write_def $def_file

diff_files mixed_ios1.defok $def_file
