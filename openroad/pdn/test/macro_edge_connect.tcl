# Macros whose supply pins are stubs on their walls, connected sideways.
#
# Every connection PDN makes is a via, and a via needs two shapes on different
# layers to overlap, so a pin that nothing crosses cannot be reached however
# close a strap runs past the macro.  None of the stubs here is crossed by
# anything: each macro blocks metal1 through metal4 over its body, which cuts
# the core metal4 straps at its outline, and no stub sits under one of the
# metal7 straps.  What connects them is the pin itself, grown along its own
# layer and out of the macro until it reaches a shape the grid has a connect
# rule to.
#
# nangate_polygon/edge_pin_macros.lef and floorplan_edge_macros.def place one
# macro of each shape, all 25.2um -- 18 rows -- tall, with the rows cut to the
# real outline of each plus a 2um margin and tap cells inserted, so nothing
# stands under a macro or in the margin around one.  The core grid runs metal4
# vertically in VSS/VDD pairs at x = 11.83 and 12.87 every 20um, and metal7
# horizontally at y = 11.1 and 14.1 on the same 20um pitch.
#
# Every macro carries a stub of both nets on every wall it has, on metal5
# where it grows east or west and metal6 where it grows north or south, so all
# four directions are built on all four shapes:
#
#   macro_rect (20 23.8)  a plain rectangle, 30 wide: four walls, eight stubs.
#                         It declares no OVERLAP obstruction, so its outline is
#                         its bounding box, as every macro in a rectangular
#                         library is.
#   macro_L    (76 37.8)  an L, 30 wide, notch at x >= 91, y >= 50.4.  Six
#                         walls: west and south are outer, east is both the far
#                         side of the foot and the wall of the notch, north is
#                         both the floor of the notch and the top of the leg.
#   macro_T    (16 60.2)  a T on its stem, 48 wide, bar at y >= 77 and stem at
#                         34 <= x <= 46.  West and east are the walls of the
#                         stem and the ends of the bar, south is the foot of
#                         the stem and the underside of the bar, north is the
#                         top of the bar.
#   macro_U    (90 77)    a U on its base, 48 wide, bend at 102 <= x <= 126,
#                         y >= 85.4.  West and east are the outer walls of the
#                         base and the inner walls of the legs, south is the
#                         foot of the base, north is the floor of the bend and
#                         the top of a leg.
#
# A pin only ever grows out of the macro.  Of its two ends the one with less
# macro beyond it is the way out; the other would be a wire back through the
# body and is not a candidate, so a pin that cannot get out the way out has no
# way out at all.  That is measured against the real outline, which is what the
# three polygon cases are for: a stub on the wall of a notch has no macro at
# all beyond it and 12 to 16.8um of bounding box, so against the bounding box
# every one of them would look like a pin buried in the middle of a macro and
# the way out would come back as the other end.
#
# macro_T carries one extra stub, VSS at y = 64.2, laid all the way across the
# stem.  It reaches the outline at both ends, so both are the way out and
# neither is settled by the outline; it goes west because the metal4 strap at
# x = 31.83 is 1.69um off that end against 5.83um to the one at x = 51.83 off
# the other.  It is also what says a strap is grown out of the end it leaves
# by rather than drawn over the whole pin: what it makes is 2.17um of metal5
# in the notch, not a wire from one side of the stem to the other.
#
# All 33 connect, so the golden holds 17 metal5 stripes and 16 metal6 ones
# that no add_pdn_stripe here asked for.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_polygon/edge_pin_macros.lef
read_def nangate_polygon/floorplan_edge_macros.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal4 -width 0.48 -spacing 0.56 -pitch 20.0 -offset 2.0
add_pdn_stripe -layer metal7 -width 1.40 -spacing 1.60 -pitch 20.0 -offset 2.0

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

define_pdn_grid -macro -name "edge_pins" \
  -instances {macro_rect macro_L macro_T macro_U}
add_pdn_connect -layers {metal4 metal5}
add_pdn_connect -layers {metal6 metal7}

pdngen

set def_file [make_result_file macro_edge_connect.def]
write_def $def_file
diff_files macro_edge_connect.defok $def_file
