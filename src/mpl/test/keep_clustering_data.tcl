# Test if groups are correctly generated.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef testcases/macro_only.lef

read_liberty Nangate45/Nangate45_fast.lib

read_def testcases/io_constraints6.def

set_thread_count 0
rtl_macro_placer -keep_clustering_data \
  -report_directory results/keep_clustering_data \
  -halo_width 4.0

set def_file [make_result_file "keep_clustering_data.def"]
write_def $def_file
diff_files $def_file "keep_clustering_data.defok"
