# A macro grid over an L-shaped hard macro.
#
# nangate_polygon/polygon_macro.lef declares
#   SIZE 57 BY 42 ;
#   OBS LAYER OVERLAP ; POLYGON 0 0 57 0 57 21 28.5 21 28.5 42 0 42 ;
# LEF has no polygonal MACRO SIZE, so SIZE is the bounding box and the
# OVERLAP-layer obstruction carries the real outline -- which is what LEF/DEF
# defines it for.  The notch x >= 28.5, y >= 21 of the bounding box is not part
# of the macro; the macro draws nothing there and no supply pin reaches into
# it.
#
# This grid has no ring and no -extend_to_core_ring, so nothing it makes has
# any business leaving the macro: every macro-grid shape has to lie inside the
# outline.  The notch is not the macro, it is free core area, and metal5 and
# metal6 over it belong to the core grid.
#
# The straps that land there are not dangling -- the golden records 8 metal4 to
# metal5 and 6 metal6 to metal7 vias inside the notch, tying them to the core
# grid's own metal4 and metal7 straps, since no macro pin reaches past y = 71.4
# to give them anything else to connect to.  That is the objection, not a
# defence of them: a macro grid has quietly annexed 28.5x21um of routing
# resource outside the macro it was defined for.
#
# Every strap now stops where the macro does.  The metal6 straps at x = 88.535
# and beyond end at y = 71.1 rather than running the full 92.865 of the
# bounding box, and the metal5 straps above y = 71.4 end at x = 79.465 rather
# than 111.8: each is cut to the length the outline gives it.
#
# Ten of the 21 still cross the boundary, but only across their width and never
# along their length.  The sweep keeps a strap whose centre is inside the
# domain, so one centred exactly on the edge -- x = 114.0 here, the right edge
# of the macro -- hangs half of its 0.93um outside.  That rule is the same on a
# rectangular macro, which is what core_grid_strap_outside_core pins, and it is
# a question about where the sweep puts a strap rather than about the outline.
#
# The metal1 rails and the metal4 and metal7 core straps in the notch are
# correct and must stay -- the notch is ordinary free core area -- and so are
# the vias between them.  A grid claims the layers it uses so that other grids
# keep off, and a via whose stack passes through a claimed layer is rejected;
# claiming the bounding box therefore rejected the core grid's metal4 to metal7
# stacks at (101.26 79.70), (91.26 89.70) and (111.26 89.70), all three of them
# in the notch where this grid is not.  Those three stacks are in the golden
# below.  The five metal4 to metal7 rejections that remain are all over the
# body of the macro, where they belong.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_polygon/polygon_macro.lef
read_def nangate_polygon/floorplan_macro.def

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
add_pdn_stripe -layer metal5 -width 0.93 -pitch 10.0 -offset 2.0
add_pdn_stripe -layer metal6 -width 0.93 -pitch 10.0 -offset 2.0

add_pdn_connect -layers {metal4 metal5}
add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal6 metal7}

pdngen

set def_file [make_result_file polygon_macro_grid.def]
write_def $def_file
diff_files polygon_macro_grid.defok $def_file
