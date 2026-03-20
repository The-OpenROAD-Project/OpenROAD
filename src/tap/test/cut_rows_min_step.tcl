source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45/fakeram45_64x7.lef
read_lef Nangate45_data/endcaps.lef
read_def cut_rows_min_step.def

place_inst -cell fakeram45_64x7 -name "test" -location {-43.5 -42} -status FIRM

cut_rows -halo_width_x 0.001 -endcap_master "ENDCAP_X4_LEFTEDGE"
place_endcaps -endcap_vertical ENDCAP_X4_LEFTEDGE

set def_file [make_result_file cut_rows_min_step.def]

check_placement -verbose

write_def $def_file
diff_file cut_rows_min_step.defok $def_file
