# Straps extended to the boundary of an L-shaped die.
#
# -extend_to_boundary resolves through Grid::getGridBoundary, which is
# block->getDieArea() -- the bounding box.  On the L-shaped die of
# nangate_polygon/floorplan.def that runs every strap the full 95x112 extent of
# the bounding box, including the part of each strap that lies in the notch
# where there is no die.
#
# A metal4 strap at x > 47.5 must stop at y = 56 and a metal7 strap at y > 56
# must stop at x = 47.5 -- and stop *on* those walls, with a pin, exactly as a
# strap reaching x = 95 or y = 112 does.  The wall of a notch is a die edge.
#
# Two things had to be true for that.  The pin is placed where a shape reaches
# an edge, and the test for "an edge" was a comparison against the bounding
# box, which no notch wall is on.  And the shape has to be able to touch the
# wall in the first place: odb marks the notch with a system-reserved
# obstruction, and a shape keeping min spacing from that stopped 0.27um short
# of it, so it never reached the wall, never earned a pin, and was then trimmed
# back to its last via at 53.28.  Nothing is owed a spacing from an absence of
# die -- there is no metal there to be clear of, and on a rectangular die none
# of these obstructions exists, which is why a strap abuts that edge freely.
#
# So the golden below has the metal4 straps at 51.26, 61.26, 71.26 and 81.26
# ending at exactly y = 56.00, and carries 5 pins on the x = 47.5 wall and 4 on
# the y = 56 wall alongside the 27 on the bounding box.
#
# Verified to hold today, by the same obstruction cut as polygon_core_grid:
# none of the 787 shapes in the result reaches the notch.  The golden should
# survive the polygon work unchanged -- and it did.  What changed is the wasted
# work, not the geometry.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_polygon/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal4 -width 0.48 -pitch 20.0 -offset 2.0 \
  -extend_to_boundary
add_pdn_stripe -layer metal7 -width 1.4 -pitch 20.0 -offset 2.0 \
  -extend_to_boundary

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

pdngen

set def_file [make_result_file polygon_core_grid_extend_to_boundary.def]
write_def $def_file
diff_files polygon_core_grid_extend_to_boundary.defok $def_file
