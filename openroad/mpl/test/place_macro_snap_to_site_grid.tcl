# A macro placed on the site grid of the rows besides the track-grid
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/macros_without_pins1.def"

place_macro -macro_name MACRO_2 -location {100.07 100.3} -snap_to_site_grid

set def_file [make_result_file place_macro_snap_to_site_grid.def]
write_def $def_file

diff_files place_macro_snap_to_site_grid.defok $def_file
