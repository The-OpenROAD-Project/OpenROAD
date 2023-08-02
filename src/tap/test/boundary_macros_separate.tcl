source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def boundary_macros.def

set def_file [make_result_file boundary_macros_separate.def]

cut_rows -endcap_master TAPCELL_X1

place_endcaps \
  -corner TAPCELL_X1 \
  -edge_corner TAPCELL_X1 \
  -endcap TAPCELL_X1

place_tapcells -master TAPCELL_X1 -distance "5"

check_placement -verbose

write_def $def_file

diff_file boundary_macros_separate.defok $def_file
