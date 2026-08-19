# Two abutting macros whose right (and left) edges are misaligned by one
# site, creating a one-row boundary jog narrower than the corner master. The
# corner at the row end must displace the overlapping inner corner so
# the jog row is not covered by both a horizontal edge fill and a row-end
# endcap.
source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def abutting_macros_step.def

set def_file [make_result_file abutting_macros_step.def]

tapcell -halo_width_x 0 -halo_width_y 0 -distance "20" \
  -tapcell_master "TAPCELL_X1" \
  -endcap_master "TAPCELL_X1" \
  -tap_nwin2_master "TAPCELL_X1" \
  -tap_nwin3_master "TAPCELL_X1" \
  -tap_nwout2_master "TAPCELL_X1" \
  -tap_nwout3_master "TAPCELL_X1" \
  -tap_nwintie_master "TAPCELL_X1" \
  -tap_nwouttie_master "TAPCELL_X1" \
  -cnrcap_nwin_master "FILLCELL_X2" \
  -cnrcap_nwout_master "FILLCELL_X2" \
  -incnrcap_nwin_master "FILLCELL_X2" \
  -incnrcap_nwout_master "FILLCELL_X2"

check_placement -verbose

write_def $def_file

diff_file abutting_macros_step.defok $def_file
