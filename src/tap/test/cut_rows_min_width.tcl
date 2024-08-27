source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def cut_rows_min_width.def

cut_rows -endcap_master "TAPCELL_X1" -row_min_width 10

set def_file [make_result_file cut_rows_min_width.def]

check_placement -verbose

write_def $def_file
diff_file cut_rows_min_width.defok $def_file
