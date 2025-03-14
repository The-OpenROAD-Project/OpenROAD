# Four ungrouped macros, centralized by the floorplan centralization
# inside SA and pushed to all corners of the core by the Pusher.
#
# Observations:
#   1. The IO pins are needed to avoid the only-macro special corner case.
#   2. The boundary weight must be manually set to zero in order
#      to prevent the floorplan centralization revert.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/orientation_improve1.lef"

read_def "./testcases/boundary_push1.def"

set_thread_count 0
rtl_macro_placer -boundary_weight 0 -report_directory results/boundary_push2 -halo_width 0.3

set def_file [make_result_file boundary_push1.def]
write_def $def_file

diff_files boundary_push1.defok $def_file