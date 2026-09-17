# Fill both horizontal boundaries across touching row fragments, selecting
# the edge master and orientation separately for each fragment.
set test_name endcap_row_fragments
source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45_data/endcaps.lef
read_def ${test_name}.def

place_endcaps \
  -corner TAPCELL_X1 \
  -endcap TAPCELL_X1 \
  -top_edge ENDCAP_X1_TOPEDGE \
  -bottom_edge ENDCAP_X1_BOTTOMEDGE

check_placement -verbose

set def_file [make_result_file ${test_name}.def]
write_def $def_file
diff_file ${test_name}.defok $def_file
