# cut_rows with a macro whose footprint is an L shape: the master SIZE is the
# 20x14 bounding box, but its obstructions only cover the L, leaving the
# bottom-right 10x7 corner free.
source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45_data/l_macro.lef
read_def cut_rows_min_step.def

# Site/row aligned: core starts at (10.07, 11.2), site is 0.19 x 1.4.
place_inst -cell l_macro -name L1 -location {40.09 39.2} -status FIRM

cut_rows -endcap_master "TAPCELL_X1"

set def_file [make_result_file cut_rows_l_macro.def]

check_placement -verbose

write_def $def_file
diff_file cut_rows_l_macro.defok $def_file
