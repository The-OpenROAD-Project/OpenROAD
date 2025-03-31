# Four ungrouped macros, centralized by the floorplan centralization
# and prevented from being pushed to the core top/bottom boundaries 
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
set_io_pin_constraint -pin_names { io_1 io_2 } -region top:*
set_io_pin_constraint -pin_names { io_3 io_4 } -region bottom:*

set_thread_count 0
rtl_macro_placer -boundary_weight 0 -report_directory results/boundary_push2 -halo_width 0.3

set def_file [make_result_file boundary_push2.def]
write_def $def_file

diff_files boundary_push2.defok $def_file