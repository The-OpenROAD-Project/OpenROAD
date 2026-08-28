# A mark that costs timing is put back rather than shipped.
#
# Swapping two cells moves their pins, which changes the wires and so the
# slack on whatever they drive.  Usually by very little -- at the default
# budget of 20 ps nothing on this design is undone -- but "usually" is not a
# guarantee, and a watermark that quietly costs a path its slack is worse than
# no watermark.
#
# The budget here is 1 ps, small enough that two pairs exceed it.  Testing at
# the default would test nothing, because a guard that never fires and a guard
# that cannot see are indistinguishable from the outside -- and this one could
# not see at all until it was given a way to re-evaluate timing.  Moving a cell
# changes only its parasitics, and nothing recomputes those until asked, so the
# check was comparing each slack against itself.
#
# Hence estimate_parasitics below: without timing set up the guard has nothing
# to compare and says so instead of pretending.
source "helpers.tcl"

foreach id { 5 6 7 8 9 392 393 1102 1103 1104 } {
  suppress_message DPL $id
}

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_placed.def
create_clock -name core_clock -period 2.0 [get_ports clk]
set_wire_rc -signal -layer metal3
estimate_parasitics -placement

set key 0011223344556677889900aabbccddeeff00112233445566778899aabbccddee
set claims [make_result_file place_guard.csv]

# 24 pairs are committed; the guard puts 2 of them back, so 22 are claimed.
set kept [place_watermark -key_hex $key -claims_file $claims \
  -hpwl_eps_um 1.0 -pair_dist_um 3.0 -pairs_per_tile 64 \
  -guard_degrade_ns 0.001]
puts "kept $kept pairs"
check "the pairs that cost timing are not claimed" { set kept } 22
check "and what is left still verifies" \
  { verify_watermark -placement_claims $claims -min_stages 1 } 1

exit_summary
