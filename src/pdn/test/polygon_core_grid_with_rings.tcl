# A ring around an L-shaped core.
#
# The core of nangate_polygon/floorplan.def is
#   (9.5 8.4) (85.5 8.4) (85.5 53.2) (43.7 53.2) (43.7 103.6) (9.5 103.6)
# inside the die
#   (0 0) (95 0) (95 56) (47.5 56) (47.5 112) (0 112)
# so the core-to-die margin is 9.5um/8.4um at the outer edges but only
# 3.8um/2.8um at the concave corner.  The ring is sized to fit that: 0.6um of
# offset plus 1.6um of metal is 2.2um against the 2.8um available.
#
# The offset is 0.6 and not less because of via spacing at the concave corner.
# At 0.2 the metal5 pad of a metal1-to-metal6 via under the rail nearest the
# corner cannot clear the 0.5um spacing to the ring's own metal5 side, and six
# vias fail as Obstructed; at 0.6 none do.  polygon_core_grid_u_rings has the
# measurement.
#
# The ring must follow the L, which has six edges, so each layer carries three
# sides and not two: metal5 the bottom, the top of the lower arm and the top of
# the tall arm, metal6 the left, the right of the lower arm and the right of the
# tall arm.  Consecutive sides have to overlap by width x width so a via lands
# there -- at the concave corner as much as at the five convex ones.
#
# The followpins carry -extend_to_core_ring, so each rail has to reach the ring
# of the arm it is in: 45.5 and 44.5 in the tall arm, 87.3 and 86.3 in the wide
# one.  Resolving that against the bounding box of all the rings instead sends
# a rail in the tall arm out across the core-to-die margin towards the wide
# arm's ring; resolving it against the core bounding box, as this did, leaves
# every rail in the tall arm at 43.7, short of the ring beside it.  Two rails
# out of 37 still stop at the core edge, both on the row boundary where the two
# arms meet and two rows share a rail.
#
# A ring built from the core bounding box instead runs through the notch, where
# the system obstructions chop it into disconnected segments and the trim pass
# then removes them: before this was polygon-aware the result was a metal5 ring
# with only a bottom side and a metal6 right side stopping at y = 51.885
# instead of 106.2, with no diagnostic.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_polygon/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1 -extend_to_core_ring

add_pdn_ring -grid "Core" -layers {metal5 metal6} -widths 0.6 -spacings 0.4 \
  -core_offsets 0.6

add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal1 metal6}

pdngen

set def_file [make_result_file polygon_core_grid_with_rings.def]
write_def $def_file
diff_files polygon_core_grid_with_rings.defok $def_file
