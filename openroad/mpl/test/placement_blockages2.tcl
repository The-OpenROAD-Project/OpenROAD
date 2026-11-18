# The annealer's location report (MPL hierarchical_macro_placement at
# debug level 2) on a design with a placement blockage: the blockage is
# represented by a soft macro with no cluster.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/macro_only.lef"

read_def "./testcases/placement_blockages1.def"

set_thread_count 0
set_debug_level MPL hierarchical_macro_placement 2
set macro [[ord::get_db_block] findInst MACRO_1]
rtl_macro_placer -report_directory [make_result_dir]
check "placed with the location report on" \
  { $macro getPlacementStatus } LOCKED

exit_summary
