# Straps extended to the boundary of a U-shaped die.
#
# polygon_core_grid_extend_to_boundary covers the L, where the die notch is cut
# out of a corner.  The U cuts it out of the middle of the top edge instead --
# nangate_polygon/floorplan_u.def has die
#   (0 0) (152 0) (152 112) (104.5 112) (104.5 56) (47.5 56) (47.5 112) (0 112)
# so the notch is x 47.5..104.5, y 56..112 -- and that changes the shape of the
# answer rather than just its numbers.
#
# On the L, a strap extended to the boundary always has one run: whichever way
# it points, the die ends once.  On the U it can have two.  A metal7 strap
# above y = 56 meets die from 0 to 47.5, then no die across the notch, then die
# again from 104.5 to 152, so extending it to the boundary has to produce two
# separate shapes and not one shape spanning the notch.  Nothing else in the
# suite exercises that: it is the only place a strap is cut into more than one
# piece by the boundary it is being extended to.
#
# The vertical straps show the other half of it.  A metal4 strap at x between
# 47.5 and 104.5 has die only up to y = 56 and must stop there, while its
# neighbours either side of the notch run the full 112.  One extend mode, three
# different answers along the same edge.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_polygon/floorplan_u.def

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

set def_file [make_result_file polygon_core_grid_u_extend_to_boundary.def]
write_def $def_file
diff_files polygon_core_grid_u_extend_to_boundary.defok $def_file
