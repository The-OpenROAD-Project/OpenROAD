# A halo around an L-shaped macro.
#
# The halo is how a macro grid tells the other grids to keep clear of it, and
# InstanceGrid::getGridRegion is the outline grown by it.  Growing an outline
# is not growing a rectangle: at the concave corner (85.5 71.4) the halo moves
# the corner inwards along both axes, so the area kept clear is the L plus 3um
# and not the bounding box plus 3um.
#
# That area is exactly what Grid::makeVias tests a via against -- a via of
# another grid whose stack passes through a layer this grid has claimed is
# rejected -- so this is also the halo half of the via question.  The core grid
# here reaches metal7 through metal5 and metal6, both of which the macro grid
# claims.
#
# So the core grid's metal4 to metal7 stacks must be rejected within 3um of the
# body of the macro and built everywhere else, the notch included: the notch is
# ordinary core area, and a halo around a macro that is not there keeps nothing
# clear.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_polygon/polygon_macro.lef
read_def nangate_polygon/floorplan_macro_halo.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal4 -width 0.48 -pitch 20.0 -offset 2.0
add_pdn_stripe -layer metal7 -width 1.4 -pitch 20.0 -offset 2.0

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

define_pdn_grid -macro -name "polygon_macro" -instances "macro_L" -halo 3
add_pdn_stripe -layer metal5 -width 0.93 -pitch 10.0 -offset 2.0
add_pdn_stripe -layer metal6 -width 0.93 -pitch 10.0 -offset 2.0

add_pdn_connect -layers {metal4 metal5}
add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal6 metal7}

pdngen

set def_file [make_result_file polygon_macro_grid_halo.def]
write_def $def_file
diff_files polygon_macro_grid_halo.defok $def_file
