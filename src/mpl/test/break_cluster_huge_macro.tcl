# Test if we handle macro-dominated partitions correctly
source "helpers.tcl"

read_lef "./Nangate45/Nangate45_tech.lef"
read_lef "./testcases/break_cluster_huge_macro.lef"

read_def "./testcases/break_cluster_huge_macro.def"

set_thread_count 0
rtl_macro_placer -max_num_macro 3 -min_num_macro 1 \
  -max_num_inst 5 -min_num_inst 2 \
  -report_directory [make_result_dir]

set def_file [make_result_file break_cluster_huge_macro.def]
write_def $def_file

diff_files break_cluster_huge_macro.defok $def_file