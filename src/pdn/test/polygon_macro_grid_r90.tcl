# The macro grid of polygon_macro_grid_with_rings with the macro turned a
# quarter.
#
# R0 and R180 both leave the axes where they are, so the bounding box of
# polygon_macro_L stays 57x42 through either and only the corner the notch sits
# in moves.  R90 swaps them: the bounding box becomes 42x57 and the outline
#   (78 25.2) (99 25.2) (99 82.2) (78 82.2) (78 53.7) (57 53.7) (57 25.2)
# puts the notch at the upper left, 21um wide and 28.5um tall rather than the
# other way round.
#
# Everything that reads the outline has to go through inst->getTransform() for
# that, and so does the halo, whose per-edge values are written against the
# macro as drawn.  An implementation that handles the mirror but not the
# quarter turn passes polygon_macro_grid_r180 and fails here.
#
# The ring must follow the turned L: three sides per layer, its concave corner
# at (78 53.7).
#
# It also pins the area a macro ring claims from other grids.
# Grid::getGridLevelObstructions marks the ring's own layers as occupied so
# that another grid's vias keep off them, and it built that mark as the
# bounding box grown by the ring offset and width -- which claims the notch
# too, where this grid has no ring at all.  The core grid's vias in the notch
# were rejected for it: 9 of them here, three stacks of
# metal4-metal5-metal6-metal7, which the golden below now carries.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_polygon/polygon_macro.lef
read_def nangate_polygon/floorplan_macro_r90.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal4 -width 0.48 -pitch 20.0 -offset 2.0
add_pdn_stripe -layer metal7 -width 1.4 -pitch 20.0 -offset 2.0 \
  -extend_to_core_ring

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

define_pdn_grid -macro -name "polygon_macro" -instances "macro_L"
add_pdn_ring -grid "polygon_macro" -layers {metal5 metal6} -widths 2.0 \
  -spacings 2.0 -core_offsets 2.0
add_pdn_stripe -layer metal5 -width 0.93 -pitch 10.0 -offset 2.0 \
  -extend_to_core_ring
add_pdn_stripe -layer metal6 -width 0.93 -pitch 10.0 -offset 2.0 \
  -extend_to_core_ring

add_pdn_connect -layers {metal4 metal5}
add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal6 metal7}

pdngen

set def_file [make_result_file polygon_macro_grid_r90.def]
write_def $def_file
diff_files polygon_macro_grid_r90.defok $def_file
