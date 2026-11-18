# The std cells at the target utilization plus the macros with their halos
# take more than 90% of the floorplan.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/macro_only.lef"

read_def "./testcases/io_constraints4.def"

set_thread_count 0
rtl_macro_placer -report_directory [make_result_dir]
