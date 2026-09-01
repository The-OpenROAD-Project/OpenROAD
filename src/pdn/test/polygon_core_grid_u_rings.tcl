# A ring around a U-shaped core: two concave corners, and a notch enclosed on
# three sides.
#
# nangate_polygon/floorplan_u.def is a 152x56 base with two 32.3um arms rising
# from its ends, so the core
#   (9.5 8.4) (142.5 8.4) (142.5 103.6) (110.2 103.6) (110.2 50.4)
#   (41.8 50.4) (41.8 103.6) (9.5 103.6)
# has six convex corners and two concave ones, at (41.8 50.4) and
# (110.2 50.4).  Everything the L fixture exercises it exercises with a single
# concave corner; this is the shape that says whether the edge walk, the corner
# convexity and the ring reach hold when there is more than one, and when the
# empty area between the arms is closed on three sides rather than open at a
# die corner.
#
# The ring must follow all eight sides -- four on each layer -- turning inwards
# at both concave corners and back out again, and every one of the eight
# corners must carry a via.
#
# And the notch between the arms must stay clear.  A rail in the left arm ends
# where that arm ends and is run out to the ring beside it, 42.6 or 43.6; the
# ring on the far side of the notch is alongside the same rail and just as
# reachable by a search that only asks which ring is furthest out, which would
# run the rail to 144.3 and straight across the notch.  Bounding the search by
# how far a ring reaches out of its own core edge is what separates the two.
#
# Three things do belong in the notch and must not be trimmed out of it:
#
#   * the ring's own sides along the notch floor, metal5 at y 50.6..51.2 and
#     51.6..52.2 -- a ring outside a concave core edge is a ring in the notch;
#   * the ring sides along the inner edge of each arm, which is why arm rails
#     reach x 42.6 and 43.6 rather than stopping at 41.8;
#   * the top rail of ROW_29 at y 50.315..50.485, spanning the full width from
#     8.7 to 143.3.  ROW_29 is the last full-width row, y 49.0..50.4, and its
#     cells need power along their top edge across that whole width, so cutting
#     this rail at the notch would leave them unpowered.  Half its 0.17um width
#     hangs above the core boundary, exactly as the rail on the core's outer
#     bottom edge hangs 0.085um below y = 8.4.
#
# Above that rail the notch has to be empty: no rail may span it and no strap
# may enter it.
#
# The ring sits 0.6um off the core, and that value is not arbitrary.  At 0.2 the
# ring's notch-floor side on metal5 ends at y = 51.2, and metal5 min spacing
# over a run that long is 0.5um, so its obstruction reaches 51.7 -- and the
# metal5 pad of a metal1-to-metal6 via beneath the rail at 51.8 cannot clear
# it.  Eight vias fail as Obstructed, and the rails that wanted them are
# extended and then trimmed back for nothing.
#
# 0.6 is an optimum rather than a floor, measured on this fixture:
#
#   offset   0.2   0.4   0.6   1.0   1.4
#   failed     8     8     0     2     2
#
# Below it the spacing is not cleared; above it the concave corner has moved
# far enough into the notch to take a second rail below the start of the ring.
# At 0.6 the ring also carries six more metal1-to-metal6 vias than at 0.2, 228
# against 222, with the same 37 rails reaching it.
#
# One rail per arm still stops at the core edge instead of reaching the ring --
# the VDD rail at y = 51.8, the first above the notch floor at 50.4.  Each ring
# net wraps the concave corner further into the notch than the last, so net 1's
# vertical side starts at 52.0, above that rail, and there is nothing beside it
# to reach.  That is geometry, not a defect, and no via is attempted for it.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_polygon/floorplan_u.def

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

set def_file [make_result_file polygon_core_grid_u_rings.def]
write_def $def_file
diff_files polygon_core_grid_u_rings.defok $def_file
