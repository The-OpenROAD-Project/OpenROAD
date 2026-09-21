# A single-layer ring around an L-shaped core.
#
# With one layer named twice, Rings::makeShapes builds both side groups on that
# same layer: the horizontal sides on the first pass and the vertical sides on
# the second.  On a rectangular core, which core_grid_with_single_layer_rings
# covers, those two groups only ever meet at the four convex corners.
#
# On a polygon core they also meet at the concave corner, and there both sides
# belong to the same layer and the same net, so the square they share is a
# same-layer overlap rather than the cross-layer one a via wants.
# GridComponent::addShape drops the loser of a colliding pair with nothing but
# a debug message, so if the concave corner is mishandled here it is mishandled
# silently.
#
# The ring must come out as six sides on metal6, meeting at the concave corner
# (43.9 53.4) without either side being dropped.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_polygon/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1 -extend_to_core_ring

add_pdn_ring -grid "Core" -layers {metal6} -widths 0.6 -spacings 0.4 \
  -core_offsets 0.2

add_pdn_connect -layers {metal1 metal6}

pdngen

set def_file [make_result_file polygon_core_grid_single_layer_ring.def]
write_def $def_file
diff_files polygon_core_grid_single_layer_ring.defok $def_file
