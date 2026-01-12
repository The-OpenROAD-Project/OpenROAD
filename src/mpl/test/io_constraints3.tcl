# Test if pin access blockages are generated correctly for a case
# with two blocked regions for pins. The macro should "escape" the
# pin blockages.
source "helpers.tcl"

# We're not interested in the connections, so don't include the lib.
read_lef "./Nangate45/Nangate45.lef"

read_lef "./testcases/macro_only.lef"
read_liberty "./testcases/macro_only.lib"

read_def "./testcases/io_constraints3.def"

exclude_io_pin_region -region right:10-125 -region top:10-150

set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir] -halo_width 4.0

set def_file [make_result_file io_constraints3.def]
write_def $def_file

diff_files io_constraints3.defok $def_file
