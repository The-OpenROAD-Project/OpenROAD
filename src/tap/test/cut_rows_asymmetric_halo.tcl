# cut_rows with an asymmetric hard halo on an L-shaped macro. Each halo side
# differs, and the halo must be applied to every OVERLAP shape of the macro:
# left 1900 (5 sites), right 5700 (15 sites), bottom 2800 (1 row),
# top 8400 (3 rows).
source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45_data/l_macro.lef
read_def cut_rows_min_step.def

# Site/row aligned: core starts at (10.07, 11.2), site is 0.19 x 1.4.
place_inst -cell l_macro -name L1 -location {40.09 39.2} -status FIRM

# setHalo takes left, bottom, right, top and a soft flag.
[[ord::get_db_block] findInst L1] setHalo 1900 2800 5700 8400 0

cut_rows -endcap_master "TAPCELL_X1"

set def_file [make_result_file cut_rows_asymmetric_halo.def]

check_placement -verbose

write_def $def_file
diff_file cut_rows_asymmetric_halo.defok $def_file
