# Followpin rails and straps on an L-shaped die and core.
#
# nangate_polygon/floorplan.def has die
#   (0 0) (95 0) (95 56) (47.5 56) (47.5 112) (0 112)
# and a core inset from it, so the notch x > 47.5, y > 56 is not part of the
# design.  odb blankets that notch with system-reserved obstructions when the
# polygon die area is set, and PDN ingests those as kBlockObs, so straps
# generated across the core *bounding box* survive only where they overlap the
# real core.
#
# The rails and straps must therefore cover the L and nothing else: no shape
# may appear in the notch, and no strap may be left as a stub that only
# reaches the bounding box edge.
#
# Verified to hold today: PDN generates the straps across the bounding box and
# the notch obstructions cut them back, so none of the 783 shapes in the result
# reaches the notch.  This is the "straps degrade gracefully" half, and the
# golden should survive the polygon work unchanged -- and it did.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_polygon/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal4 -width 0.48 -pitch 20.0 -offset 2.0
add_pdn_stripe -layer metal7 -width 1.4 -pitch 20.0 -offset 2.0

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

pdngen

set def_file [make_result_file polygon_core_grid.def]
write_def $def_file
diff_files polygon_core_grid.defok $def_file
