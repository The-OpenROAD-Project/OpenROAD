# Test if pin access blockages are generated correctly for a case
# with both constrained and unconstrained pins. There are no blocked
# regions for pins.
source "helpers.tcl"

# We're not interested in the connections, so don't include the lib.
read_lef "./Nangate45/Nangate45.lef"

read_lef "./testcases/macro_only.lef"
read_liberty "./testcases/macro_only.lib"

read_def "./testcases/io_constraints1.def"

set_io_pin_constraint -pin_names {io_1 io_2} -region right:70-90

set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir] -halo_width 4.0

set def_file [make_result_file io_constraints9.def]
write_def $def_file

diff_files io_constraints9.defok $def_file
