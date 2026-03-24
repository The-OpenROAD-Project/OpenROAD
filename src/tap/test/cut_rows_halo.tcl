source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def boundary_macros.def

# Create halo
odb::dbBox_create [[ord::get_db_block] findInst memNE] 2000 2000 2000 2000

cut_rows -endcap_master "TAPCELL_X1"

set def_file [make_result_file cut_rows_halo.def]

check_placement -verbose

write_def $def_file
diff_file cut_rows_halo.defok $def_file
