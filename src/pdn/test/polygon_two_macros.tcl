# Two polygon macros on a U-shaped die, one in each arm.
#
# nangate_polygon/floorplan_two_macros.def gives the U-shaped die a core whose
# arms are exactly as wide as the macro that fills each of them, and mirrors
# the right macro so the two notches face each other across the gap:
#
#   die          (0 0) (152 0) (152 112) (104.5 112) (104.5 56) (47.5 56)
#                (47.5 112) (0 112)
#   core         (9.5 8.4) (142.5 8.4) (142.5 103.6) (114 103.6) (114 50.4)
#                (38 50.4) (38 103.6) (9.5 103.6)
#   macro_left   bbox (9.5 33.6) - (38 103.6), R0, notch at its upper right
#   macro_right  bbox (114 33.6) - (142.5 103.6), MY, notch at its upper left
#
# Each macro fills its arm from side to side and runs to the top of the core,
# so the area it occupies is not a hole in the row footprint but a bite out of
# the boundary, and filling holes does not recover it.  What is left of the
# footprint is three disjoint islands -- 30 rows across the base, 16 in each
# macro notch -- so this is the case that needs
# VoltageDomain::getDomainRegion to unite the outlines of the macros standing
# in the core with the rows.  Every other fixture here has one macro; two is
# what exercises the accumulation.
#
# Without that union the footprint fails the one-piece well-formedness check
# and the domain falls back to the core bounding box, which is the whole U
# including the gap between the arms.  Measured: 9 of the metal4 straps then
# run past the top of the core at y = 50.4 and stop at y = 56, which is not
# the core boundary but the floor of the die notch, where odb's obstruction
# begins; and all 10 metal7 straps in the arms reach the wall of the die notch
# at x = 47.5 and 104.5 rather than stopping at the core arm.  Both hold now.
#
# The macros also pin CoreGrid::cleanupShapes, which drops core shapes that
# lie inside a macro.  Measured against the bounding-box version of that test:
# 33 followpin rails rather than 65.  Every rail in a macro notch except the
# topmost was dropped as being inside the macro -- the topmost survives only
# because it straddles the core boundary and so pokes 0.085um past the bbox --
# together with two of the metal4 straps crossing the notches.  Those rails sit
# over real rows and connect to the straps above them.
#
# The rows are cut half a row clear of the macro bodies in Y.  A row that abuts
# a body exactly puts its rail half over the macro's own metal1, and the rail
# on the floor of a notch has no metal4 above it either, because the body
# blocks metal1 through metal5 and a strap crossing the notch cannot start
# until a spacing past it.  That is an ordinary macro keepout question, not a
# polygon one, and without the cut it is an unrepairable channel (PDN-0179).
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_polygon/polygon_macro_tall.lef
read_def nangate_polygon/floorplan_two_macros.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal4 -width 0.48 -pitch 15.0 -offset 2.0
add_pdn_stripe -layer metal7 -width 1.4 -pitch 20.0 -offset 2.0

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

define_pdn_grid -macro -name "tall_macros" -instances "macro_left macro_right"
add_pdn_stripe -layer metal6 -width 0.93 -pitch 10.0 -offset 2.0

add_pdn_connect -layers {metal6 metal7}

pdngen

set def_file [make_result_file polygon_two_macros.def]
write_def $def_file
diff_files polygon_two_macros.defok $def_file
