# Four ungrouped macros, centralized by the floorplan centralization
# and prevented from being pushed to the core left/right boundaries 
# by pin access blockages along those boundaries.
#
# Observations:
#   1. The std cells are needed, because the blockages' depth is
#      computed based on their area.
#   2. The boundary weight must be manually set to zero in order
#      to prevent the floorplan centralization revert.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/boundary_push2.def"

# Blockages are created based on IOs constraints
set_io_pin_constraint -pin_names { io_1 io_2 } -region left:*
set_io_pin_constraint -pin_names { io_3 io_4 } -region right:*

set_thread_count 0
rtl_macro_placer -boundary_weight 0 -report_directory results/boundary_push3 -halo_width 0.3

set def_file [make_result_file boundary_push3.def]
write_def $def_file

diff_files boundary_push3.defok $def_file