# A ring around an L-shaped hard macro.
#
# The macro of nangate_polygon/floorplan_macro.def occupies the L
#   (57 50.4) (114 50.4) (114 71.4) (85.5 71.4) (85.5 92.4) (57 92.4)
# inside the bounding box (57 50.4) - (114 92.4).  Rings::getInnerRingOutline
# offsets Grid::getDomainArea, which for an instance grid is the bounding box,
# so the ring is four rectangles around 57x42 instead of eight around the L.
#
# Nothing obstructs the notch here -- it is ordinary free core area -- so
# unlike the polygon-die case there is no obstruction cut to hide the error.
# The ring simply encloses 28.5x21um of core that does not belong to the macro,
# blocking metal5/metal6 over it and leaving the concave corner of the macro
# with no ring metal at all.
#
# The ring must follow the L: eight sides per layer, joined at the concave
# corner, and the core area inside the notch must stay available to the core
# grid.
#
# The straps here carry -extend_to_core_ring, so unlike polygon_macro_grid
# they are meant to reach past the macro and land on the ring; it is the
# ring they land on that has to follow the outline.
#
# It does now: the golden below records three sides per layer, joined at the
# concave corner (87.5 73.4) where a 2x2um overlap carries the via, so the ring
# no longer encloses the 28.5x21um of core that is not part of the macro.
#
# The straps are a separate matter and still cross the notch: they carry
# -extend_to_core_ring, so they are meant to reach past the macro to the ring,
# but they are generated across the bounding box rather than the outline.  That
# is what polygon_macro_grid pins.
#
# It also pins the area a macro ring claims from other grids.
# Grid::getGridLevelObstructions marks the ring's own layers as occupied so
# that another grid's vias keep off them, and it built that mark as the
# bounding box grown by the ring offset and width -- which claims the notch
# too, where this grid has no ring at all.  The core grid's vias in the notch
# were rejected for it: 9 of them here, three stacks of
# metal4-metal5-metal6-metal7, which the golden below now carries.  Two metal7
# straps reach x = 121.740 and 111.740 rather than 114.465 and 109.465, each
# held by one of the new vias.
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
add_pdn_stripe -layer metal7 -width 1.4 -pitch 20.0 -offset 2.0 \
  -extend_to_core_ring

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

define_pdn_grid -macro -name "polygon_macro" -instances "macro_L"
add_pdn_ring -grid "polygon_macro" -layers {metal5 metal6} -widths 2.0 \
  -spacings 2.0 -core_offsets 2.0
add_pdn_stripe -layer metal5 -width 0.93 -pitch 10.0 -offset 2.0 \
  -extend_to_core_ring
add_pdn_stripe -layer metal6 -width 0.93 -pitch 10.0 -offset 2.0 \
  -extend_to_core_ring

add_pdn_connect -layers {metal4 metal5}
add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal6 metal7}

pdngen

set def_file [make_result_file polygon_macro_grid_with_rings.def]
write_def $def_file
diff_files polygon_macro_grid_with_rings.defok $def_file
