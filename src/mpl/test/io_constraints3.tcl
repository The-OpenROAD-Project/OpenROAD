# Test if pin access blockages are generated correctly for a case
# with two blocked regions for pins.
source "helpers.tcl"

# We're not interested in the connections, so don't include the lib.
read_lef "./Nangate45/Nangate45.lef"

read_lef "./testcases/macro_only.lef"
read_liberty "./testcases/macro_only.lib"

read_verilog "./testcases/io_constraints1.v"
link_design "io_constraints1"
read_def "./testcases/io_constraints1.def" -floorplan_initialize

exclude_io_pin_region -region right:10-125 -region top:10-150

set_thread_count 0
rtl_macro_placer -report_directory results/io_constraints3 -halo_width 4.0

set def_file [make_result_file io_constraints3.def]
write_def $def_file

diff_files io_constraints3.defok $def_file