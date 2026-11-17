# The std cells at the target utilization plus the macros with their halos
# do not fit in the floorplan.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/macro_only.lef"

read_def "./testcases/io_constraints4.def"

catch { rtl_macro_placer -target_util 0.2 -report_directory [make_result_dir] }
