# A ring whose two layers carry very different amounts of metal, on a
# rectangular die, where the room for it differs between X and Y.
#
# This pins which of the two totals bounds which axis.  getTotalWidth returns
# the horizontal layer's stacked metal in `hor` and the vertical layer's in
# `ver`, and it is `ver` that bounds X: makeShapes sweeps each side outward by
# the width of its own layer, so the left and right sides -- the vertical
# layer's -- are what reach out in X.
#
# nangate_gcd/floorplan.def leaves 9.88um between the core and the die on the
# right and 11.2um above and below, and each ring here reaches 11.0um out of
# the core on one axis and 2.0um on the other.  So the same two numbers, read
# one way round or the other, decide whether the ring fits:
#
#   metal5 0.5/0.5, metal6 4.5/1.5 -> 11.0um in X against 9.88um: does not fit,
#     and PDN-0351 says so, by 1.12um in X;
#   metal5 4.5/1.5, metal6 0.5/0.5 -> 11.0um in Y against 11.2um and 2.0um in X
#     against 9.88um: fits, with 0.2um to spare, and the golden below is the
#     ring being built.
#
# Rings::checkDieArea used to read them the other way round, which swapped both
# verdicts exactly.  Measured on that version: the ring that fits was rejected
# with the same "by 1.12 um in X" the overrunning one earns now, and the
# overrunning one raised nothing at all -- not even the -allow_out_of_die
# warning -- and pdngen built it, leaving four ring shapes outside the die from
# x = -0.93 to 101.25 against a die of 0 to 100.13.
#
# polygon_core_grid_rings_asymmetric is the polygon half of the same question,
# where the transposition changed the numbers in the message rather than the
# verdict.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1

# the vertical layer carries the metal, so the ring is 11.0um wide in X
catch {
  add_pdn_ring -grid "Core" -layers {metal5 metal6} -widths "0.5 4.5" \
    -spacings "0.5 1.5" -core_offsets 0.5
} err
puts $err

# the horizontal layer carries it instead, so the same metal reaches out in Y,
# where there is room for it
add_pdn_ring -grid "Core" -layers {metal5 metal6} -widths "4.5 0.5" \
  -spacings "1.5 0.5" -core_offsets 0.5

add_pdn_connect -layers {metal1 metal5}
add_pdn_connect -layers {metal5 metal6}

pdngen

set def_file [make_result_file core_grid_with_rings_asymmetric_widths.def]
write_def $def_file
diff_files core_grid_with_rings_asymmetric_widths.defok $def_file
