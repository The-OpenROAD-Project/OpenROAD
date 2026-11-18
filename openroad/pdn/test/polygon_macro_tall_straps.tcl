# Core straps cut by a tall L-shaped macro.
#
# nangate_polygon/polygon_macro_tall.lef is 28.5x84 with the body
#   (0 0) (28.5 0) (28.5 56) (14.25 56) (14.25 84) (0 84)
# and it blocks metal1 through metal5 over that body, carrying its supply pins
# on metal6.  Placed at (47.5 9.8) in a 9.5..123.5 x 8.4..103.6 core it stands
# 84um high against a 95.2um core, so a core strap that meets it has nowhere to
# go around: it has to be cut.
#
# The macro body is
#   (47.5 9.8) - (76 65.8)  and  (47.5 65.8) - (61.75 93.8)
# so the notch is (61.75 65.8) - (76 93.8).  The vertical metal4 core straps
# then split into two populations:
#
#   * a strap at x in 47.5..61.75 meets the body over the whole 9.8..93.8
#     height and is cut into a stub below and a stub above the macro;
#   * a strap at x in 61.75..76 meets the body only up to y = 65.8 and then
#     runs on through the notch to the top of the core.
#
# The second population is the one that distinguishes the outline from the
# bounding box, and it is what would be lost if the obstruction a macro
# contributes to other grids were taken from inst->getBBox().  The metal1 rails
# behave the same way: none under the body, because the rows were cut there,
# but present in the notch, which is ordinary placeable area.
#
# The metal4 half already holds today: the golden below records the strap at
# x = 71.26..71.74 running 67.115..103.685 through the notch and the one at
# x = 51.26..51.74 cut to stubs at 8.315..16.885 and 90.010..103.685, because
# InstanceGrid::getInstanceObstructions walks the per-layer decomposed polygon
# obstructions.  That half is a guard on the polygon work, not a defect.
#
# The metal6 half holds now too: the three macro-grid straps at x = 64.035,
# 69.035 and 74.035 used to run to y = 81.1 over the notch, past the last macro
# metal6 pin at y = 65.8 and so connected to nothing.  None of the six metal6
# straps leaves the outline any more.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_polygon/polygon_macro_tall.lef
read_def nangate_polygon/floorplan_macro_tall.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal4 -width 0.48 -pitch 20.0 -offset 2.0
add_pdn_stripe -layer metal7 -width 1.4 -pitch 20.0 -offset 2.0

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

define_pdn_grid -macro -name "polygon_macro" -instances "macro_L"
add_pdn_stripe -layer metal6 -width 0.93 -pitch 10.0 -offset 2.0

add_pdn_connect -layers {metal6 metal7}

pdngen

set def_file [make_result_file polygon_macro_tall_straps.def]
write_def $def_file
diff_files polygon_macro_tall_straps.defok $def_file
