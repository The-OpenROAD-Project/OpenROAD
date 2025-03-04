# init_floorplan -utilization -aspect_ratio -core_space
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def make_rows_no_rows.def 

make_rows -core_space 1 -site FreePDK45_38x28_10R_NP_162NW_34O
set def_file [make_result_file make_rows.def]
write_def $def_file
diff_files make_rows.defok $def_file
