# Test if pin access blockages are generated correctly for a case
# with pins with different constraint regions. The region on the left
# edge has more pins and should have a larger blockage.
# The macro should be placed closer to the right edge.
source "helpers.tcl"

# We're not interested in the connections, so don't include the lib.
read_lef "./Nangate45/Nangate45.lef"

read_lef "./testcases/macro_only.lef"
read_liberty "./testcases/macro_only.lib"

read_verilog "./testcases/io_constraints1.v"
link_design "io_constraints1"
read_def "./testcases/io_constraints1.def" -floorplan_initialize

set_io_pin_constraint -pin_names {io_1 io_2} -region left:70-90
set_io_pin_constraint -pin_names {io_3} -region right:70-90

set_thread_count 0
rtl_macro_placer -report_directory results/io_constraints4 -halo_width 4.0

set def_file [make_result_file io_constraints4.def]
write_def $def_file

diff_files io_constraints4.defok $def_file