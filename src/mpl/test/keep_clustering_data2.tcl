# Test if the macros of a module are kept out of the group
# generated for the module's std cell cluster.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef testcases/macro_only.lef

read_verilog testcases/keep_clustering_data2.v
link_design keep_clustering_data2

initialize_floorplan -die_area "0 0 400 400" \
  -core_area "10 10 390 390" \
  -site FreePDK45_38x28_10R_NP_162NW_34O

# Tracks must exist otherwise the snapper will fail.
make_tracks

set_thread_count 0

# We use custom cluster thresholds to enforce root splitting.
rtl_macro_placer -keep_clustering_data \
  -max_num_inst 6 \
  -min_num_inst 2 \
  -max_num_macro 2 \
  -min_num_macro 1 \
  -report_directory results/keep_clustering_data2

set def_file [make_result_file "keep_clustering_data2.def"]
write_def $def_file
diff_files $def_file "keep_clustering_data2.defok"
