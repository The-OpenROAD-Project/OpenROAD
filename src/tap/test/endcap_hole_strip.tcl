# Rows leave an enclosed void (hole) and a void open to the core edge (notch)
# side by side, separated by a narrow strip of rows whose top is flush with
# the top of both voids.  Endcaps along the bottom edge of the rows above the
# voids must land in the row above the voids, not in the strip's row.
source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def endcap_hole_strip.def

set def_file [make_result_file endcap_hole_strip.def]

place_endcaps \
  -corner TAPCELL_X1 \
  -edge_corner TAPCELL_X1 \
  -endcap TAPCELL_X1

check_placement -verbose

write_def $def_file

diff_file endcap_hole_strip.defok $def_file
