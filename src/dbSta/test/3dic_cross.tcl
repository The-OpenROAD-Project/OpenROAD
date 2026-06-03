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

proc chip_net_names { } {
  set names {}
  foreach n [get_nets *] {
    lappend names [get_full_name $n]
  }
  return [lsort $names]
}
check "chip-net set" { chip_net_names } {bridge clk_top in_top out_top}
check "bridge spans both chiplets" {
  set insts {}
  foreach pin [get_pins -of_objects [get_nets bridge]] {
    lappend insts [get_full_name [$pin instance]]
  }
  lsort -unique $insts
} {chipA chipB}

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
