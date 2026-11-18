# A ring hugging a core whose corners are cut away, in a padring.
#
# nangate_polygon/floorplan_cut_corners_pads.def is a 570x571.2 die with a
# 140um pad row on each side and the core
#   (199.5 162.4) (370.5 162.4) (370.5 201.6) (408.5 201.6) (408.5 369.6)
#   (370.5 369.6) (370.5 408.8) (199.5 408.8) (199.5 369.6) (161.5 369.6)
#   (161.5 201.6) (199.5 201.6)
# which is a 247x246.4 rectangle with a 38x39.2 bite out of each corner.  Every
# cut turns one convex corner into two convex corners and one concave one, so
# there are four concave corners here, one facing each way -- the L fixture has
# one and the U two, and both of those face the same way.
#
# Two things have to hold at once.
#
# The ring has to hug the core: twelve sides, six on each layer, turning in and
# out again at each cut, rather than four sides around the bounding box.  A
# bounding-box ring would run diagonally past all four cuts and sit 38um from
# the core there.
#
# And the pads still have to reach it.  Each side carries two pads, one facing
# a cut and one facing the middle of the edge, and the ring is not the same
# distance from both, so -connect_to_pads has to find the ring where it
# actually is.  On the bottom edge that comes out as two connections of very
# different lengths:
#
#   * pad_vdd_s, at x 170..195, faces the cut, and its straps run from the pad
#     edge at y 139.9 up to y = 199.6 -- the horizontal face of the cut;
#   * pad_vss_s, at x 280..305, faces the middle, and reaches y = 153.4.
#
# 59.7um against 13.5um, to two different sides of the same ring.  Resolving
# the ring position once for the grid instead of per pad gets one of the two
# wrong by 46um.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_bsg_black_parrot/dummy_pads.lef
read_def nangate_polygon/floorplan_cut_corners_pads.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VDD -pin_pattern {^DVDD$}
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
add_global_connection -net VSS -pin_pattern {^DVSS$}

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core" -starts_with "POWER"
add_pdn_ring -grid "Core" -layers {metal8 metal9} -widths 5.0 \
  -spacings 2.0 -core_offsets 2 -connect_to_pads

add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal4 -width 0.48 -pitch 56.0 -offset 2.24 \
  -extend_to_core_ring
add_pdn_stripe -layer metal7 -width 1.40 -pitch 40.0 -offset 2.70 \
  -extend_to_core_ring
add_pdn_stripe -layer metal8 -width 1.40 -pitch 40.0 -offset 2.70 \
  -extend_to_core_ring
add_pdn_stripe -layer metal9 -width 1.40 -pitch 40.0 -offset 2.70 \
  -extend_to_core_ring

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}
add_pdn_connect -layers {metal7 metal8}
add_pdn_connect -layers {metal8 metal9}

pdngen

set def_file [make_result_file polygon_cut_corners_pads.def]
write_def $def_file
diff_files polygon_cut_corners_pads.defok $def_file
