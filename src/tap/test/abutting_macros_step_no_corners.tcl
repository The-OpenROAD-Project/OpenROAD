# Two abutting macros with edges misaligned by one site, run without corner
# masters. With no corner cell to terminate the jog row, the horizontal edge
# fills and the row-end endcaps must not be placed on top of each other, at
# the jog or at the die corners.
source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def abutting_macros_step.def

set def_file [make_result_file abutting_macros_step_no_corners.def]

tapcell -halo_width_x 0 -halo_width_y 0 -distance "20" \
  -tapcell_master "TAPCELL_X1" \
  -endcap_master "TAPCELL_X1" \
  -tap_nwin2_master "TAPCELL_X1" \
  -tap_nwin3_master "TAPCELL_X1" \
  -tap_nwout2_master "TAPCELL_X1" \
  -tap_nwout3_master "TAPCELL_X1" \
  -tap_nwintie_master "TAPCELL_X1" \
  -tap_nwouttie_master "TAPCELL_X1"

check_placement -verbose

write_def $def_file

diff_file abutting_macros_step_no_corners.defok $def_file
