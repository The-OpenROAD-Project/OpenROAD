# A ring around a T-shaped core: two concave corners facing down, and two
# separate notches.
#
# nangate_polygon/floorplan_t.def is a 152x56 bar across the top of the die
# with a 57x56 stem below its middle, so the core
#   (57 8.4) (95 8.4) (95 61.6) (142.5 61.6) (142.5 103.6) (9.5 103.6)
#   (9.5 61.6) (57 61.6)
# has six convex corners and two concave ones, at (57 61.6) and (95 61.6).
#
# polygon_core_grid_u_rings is the same corner count the other way up, and the
# difference is what this fixture is for.  In the U both concave corners sit on
# one horizontal edge, so the notch between the legs is a single pocket bounded
# by both of them and the ring crosses it in one run.  In the T each concave
# corner bounds a pocket of its own -- one either side of the stem -- so the
# complement of the core is disconnected, and the edge the ring has to break
# into two runs is the bottom rather than the top.
#
# The ring follows all eight sides on each layer, four per net:
#
#   * metal5 carries the stem bottom at x 54.800..97.200, the bar top across
#     the full width at 7.300..144.700, and the underside of the bar in two
#     pieces, 7.300..55.400 and 96.600..144.700, one per pocket.  The inner net
#     repeats all four one micron in.
#   * metal6 carries the two outer walls and the two stem walls.
#
# Every one of the eight corners carries a via; 696 in all.
#
# The underside sides sit at y 59.400..60.000 and 60.400..61.000, which is
# below the core boundary at 61.600: a ring outside a concave core edge is a
# ring in the notch, and it must not be trimmed out of one.
#
# The rails come to 69.  Thirty-eight of them are in the stem and run out to
# the ring beside it, to 54.800 and 97.200 or 55.800 and 96.200; thirty-one
# cross the bar and reach 7.300 and 144.700 or 8.300 and 143.700.
#
# One rail is shared.  The stem's top row and the bar's bottom row meet at
# y = 61.6, so the rail there belongs to both: it spans the full bar width,
# 8.300..143.700, and half of its 0.17um width hangs below the core boundary,
# exactly as the U's ROW_29 rail hangs above its notch floor.  The cells along
# the bottom of the bar need power across that whole width, so cutting it back
# to the stem would leave them unpowered.
#
# Below that rail both pockets must stay empty, and they do: no rail under the
# bar reaches past the ring beside the stem.
#
# The ring reaches 2.2um out of the core, against 9.5um of clearance at the
# stem walls and 5.6um at the concave corners, so PDN-0351 has nothing to say
# here -- polygon_core_grid_rings_outside_die is where it does.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_polygon/floorplan_t.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1 -extend_to_core_ring

add_pdn_ring -grid "Core" -layers {metal5 metal6} -widths 0.6 -spacings 0.4 \
  -core_offsets 0.6

add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal1 metal6}

pdngen

set def_file [make_result_file polygon_core_grid_t_rings.def]
write_def $def_file
diff_files polygon_core_grid_t_rings.defok $def_file
