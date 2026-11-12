# Macros snapped to the site grid of the rows besides the track-grid
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/macros_without_pins1.def"

set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir] -snap_to_site_grid

set def_file [make_result_file snap_to_site_grid.def]
write_def $def_file

diff_files snap_to_site_grid.defok $def_file
