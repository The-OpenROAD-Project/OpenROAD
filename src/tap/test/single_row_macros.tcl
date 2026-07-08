# Two macros stacked with a single row between them. That row is both the
# "Top" edge (macro above) and the "Bottom" edge (macro below); tapcell must
# not place overlapping endcap edge/corner cells in it.
source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def single_row_macros.def

set def_file [make_result_file single_row_macros.def]

tapcell -halo_width_x 0 -halo_width_y 0 -distance "20" \
  -tapcell_master "TAPCELL_X1" \
  -endcap_master "TAPCELL_X1" \
  -tap_nwin2_master "TAPCELL_X1" \
  -tap_nwin3_master "TAPCELL_X1" \
  -tap_nwout2_master "TAPCELL_X1" \
  -tap_nwout3_master "TAPCELL_X1" \
  -tap_nwintie_master "TAPCELL_X1" \
  -tap_nwouttie_master "TAPCELL_X1" \
  -cnrcap_nwin_master "TAPCELL_X1" \
  -cnrcap_nwout_master "TAPCELL_X1" \
  -incnrcap_nwin_master "TAPCELL_X1" \
  -incnrcap_nwout_master "TAPCELL_X1"

check_placement -verbose

write_def $def_file

diff_file single_row_macros.defok $def_file
