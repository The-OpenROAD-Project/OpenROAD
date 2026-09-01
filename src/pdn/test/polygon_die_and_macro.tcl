# An L-shaped macro on an L-shaped die.
#
# nangate_polygon/floorplan_polygon_macro.def tucks polygon_macro_L into the
# corner of the L-shaped die, so that the *bounding box* of the macro,
# (11.4 30.8) - (68.4 72.8), reaches 20.9um into the die notch while the body
# of the macro does not:
#
#   body   (11.4 30.8) (68.4 30.8) (68.4 51.8) (39.9 51.8) (39.9 72.8)
#          (11.4 72.8)
#   die    (0 0) (95 0) (95 56) (47.5 56) (47.5 112) (0 112)
#
# Every part of the macro is inside the die.  Reasoning in bounding boxes says
# otherwise, and the two polygons interact: the macro grid extends over the
# notch, where odb has put system-reserved obstructions on every routing layer,
# so the macro straps and ring in that region are cut away rather than simply
# being wasted.
#
# The macro grid must stay inside the outline of the macro, the core grid must
# keep the area between the macro and the die edge, and no shape may land in
# the die notch.
#
# Today 16 of the 21 macro-grid straps leave the macro outline and 14 of those
# leave its bounding box, the same way as in polygon_macro_grid.
#
# Both polygons are honoured now: the core ring follows the L, three sides per
# layer, as in polygon_core_grid_with_rings, and the macro grid keeps to the
# outline of the macro.  What is left is the width straddle described in
# polygon_macro_grid -- 12 of the 19 macro straps sit centred on an edge -- and
# that is the sweep's rule, not the outline's.
#
# The die notch itself has been right all along: none of the shapes reaches it,
# because odb obstructs it.
#
# The macro also pins CoreGrid::cleanupShapes, which drops core shapes that lie
# inside a macro.  Measured against the bounding-box version of that test: 13
# followpin rails in the notch of the macro, at x = 39.900 - 45.900, and the 81
# vias that go with them, were dropped as being inside the macro, and the
# metal4 strap at x = 41.500 stopped at y = 59.700 rather than reaching down to
# y = 54.515, having lost the rails it connects to there.  Those rails sit over
# real rows.  polygon_two_macros measures the same thing on a macro that fills
# its arm.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_polygon/polygon_macro.lef
read_def nangate_polygon/floorplan_polygon_macro.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1 -extend_to_core_ring
add_pdn_stripe -layer metal4 -width 0.48 -pitch 20.0 -offset 2.0
add_pdn_ring -grid "Core" -layers {metal5 metal6} -widths 0.6 -spacings 0.4 \
  -core_offsets 0.6
add_pdn_stripe -layer metal7 -width 1.4 -pitch 20.0 -offset 2.0

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal1 metal6}
add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal4 metal7}

define_pdn_grid -macro -name "polygon_macro" -instances "macro_L"
add_pdn_stripe -layer metal5 -width 0.93 -pitch 10.0 -offset 2.0
add_pdn_stripe -layer metal6 -width 0.93 -pitch 10.0 -offset 2.0

add_pdn_connect -layers {metal4 metal5}
add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal6 metal7}

pdngen

set def_file [make_result_file polygon_die_and_macro.def]
write_def $def_file
diff_files polygon_die_and_macro.defok $def_file
