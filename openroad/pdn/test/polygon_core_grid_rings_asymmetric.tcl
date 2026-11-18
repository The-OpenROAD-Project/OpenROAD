# A ring whose two layers carry different amounts of metal, overrunning a
# polygon die.
#
# This is the case that pins which axis each of the two totals belongs to.
# getTotalWidth returns the horizontal layer's stacked metal in `hor` and the
# vertical layer's in `ver`, and it is `ver` that grows X: makeShapes sweeps
# each side outward by the width of its own layer, so the bottom and top --
# the horizontal layer's sides -- add their thickness to Y.
#
# Rings::checkDieArea had these the other way round, and no test could see it.
# core_grid_with_rings_different_width1 does give hor = 4 and ver = 5, but its
# ring fits, and checkDieArea says nothing about a ring that fits.  Here, with
# metal5 at 1.0/2.0 and metal6 at 2.0/1.0, the same hor = 4 and ver = 5 have to
# be reported as a deficit:
#
#   * against the die bounding box the ring fits -- 2.0 + 5.0 of X against a
#     9.5um margin, 2.0 + 4.0 of Y against 8.4um -- so this also re-pins that
#     the guard is looking at the outline;
#   * at the concave corner the margins are 3.8um in X and 2.8um in Y, so the
#     deficits are 7.0 - 3.8 = 3.2 in X and 6.0 - 2.8 = 3.2 in Y.
#
# Transposing the two totals reports 2.2 and 4.2 instead, so the message below
# is what distinguishes the two conventions.  The rectangular half of that
# transposition -- where it silently allowed a ring outside the die instead of
# reporting one that fits -- is core_grid_with_rings_asymmetric_widths.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_polygon/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1 -extend_to_core_ring

catch {
  add_pdn_ring -grid "Core" -layers {metal5 metal6} -widths "1.0 2.0" \
    -spacings "2.0 1.0" -core_offsets 2.0 -add_connect
} err
puts $err
