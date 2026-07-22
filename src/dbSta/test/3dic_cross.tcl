source "helpers.tcl"

# Cross-chiplet timing path. Two distinct chiplet masters (flop_chip_a,
# flop_chip_b), each with DFF + INV + BUF, wired top-side so chipA.q ->
# bridge -> chipB.d. buildChipNetsFromVerilog reads 3dic_cross_top.v and
# creates a dbChipNet per top-level wire. STA traverses via term()
# (Stage 4 bridge) + DbNetPinIterator (Stage 5) + visitConnectedPins
# chip-net descent (Stage 7) so wire edges form across the boundary.
set_thread_count 1

read_3dbx 3dic_cross.3dbx

# Stage 8: structural summary helper. Exercises the dbChip/dbChipNet/
# dbChipConn iteration paths and surfaces user-facing counts in 3DBlox
# terminology.
report_3dic_summary

# Unique-master structural iteration: each chiplet master (flop_chip_a,
# flop_chip_b) is instantiated exactly once, so DbInstanceChildIterator
# surfaces both chip-insts AND flat-descends their unique interiors (ff/inv/
# buf + the bump insts). This is the A3 unique-master descent. (Duplicated
# masters are rejected up front by STA-3004; see 3dic_get_cells.tcl.)
proc cell_names { } {
  set names {}
  foreach c [get_cells *] {
    lappend names [get_full_name $c]
  }
  return [lsort $names]
}
check "flat cell set" { cell_names } {chipA chipA/buf chipA/bump_clk\
  chipA/bump_d chipA/bump_q chipA/ff chipA/inv chipB chipB/buf\
  chipB/bump_clk chipB/bump_d chipB/bump_q chipB/ff chipB/inv}
# A bump's pin is its pad inst's single iterm (the chip-inst itself is
# pin-less) -- query pins through the pad leaf insts.
check "chipA bump-pin count" {
  llength [get_pins -of_objects \
    [get_cells {chipA/bump_clk chipA/bump_d chipA/bump_q}]]
} 3
check "chipB bump-pin count" {
  llength [get_pins -of_objects \
    [get_cells {chipB/bump_clk chipB/bump_d chipB/bump_q}]]
} 3

proc chip_net_names { } {
  set names {}
  foreach n [get_nets *] {
    lappend names [get_full_name $n]
  }
  return [lsort $names]
}
check "chip-net set" { chip_net_names } {bridge clk_top in_top out_top}
# A chip-net's pins are the bump pad iterms; each pad inst belongs to its
# chiplet, so the net provably spans BOTH chiplets.
check "bridge spans both chiplets" {
  set insts {}
  foreach pin [get_pins -of_objects [get_nets bridge]] {
    lappend insts [get_full_name [$pin instance]]
  }
  lsort -unique $insts
} {chipA/bump_q chipB/bump_d}

# Anchor the clock on the chip-bump pins of clk_top — the natural form
# users will write. Each chip-bump pin reports BIDIRECT (dbNetwork::
# direction), so Graph::makePinVertices gives it both a load and a driver
# vertex and create_clock seeds the arrival on both. The driver vertex then
# fans out into the chiplet body via the fat-net wire-edge model, so the
# clock reaches each chiplet's internal CK with no synthesized timing arc.
create_clock -name clk -period 1.0 \
  [get_pins -of_objects [get_nets clk_top]]
report_checks -path_delay max -group_path_count 4

exit_summary
