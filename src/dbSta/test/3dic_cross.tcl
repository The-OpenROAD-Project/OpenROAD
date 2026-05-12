source "helpers.tcl"

# Stage 6: end-to-end cross-chiplet timing path. Two instances of
# flop_chip (DFF inside each) wired top-side so chipA.q -> chipB.d.
# buildChipNetsFromVerilog reads 3dic_cross_top.v and creates a
# dbChipNet per top-level wire. STA's graph builder traverses via
# term() (Stage 4) + DbNetPinIterator (Stage 5).
set_thread_count 1

read_3dbx 3dic_cross.3dbx

# Verify chip-nets were built from the top verilog.
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

# Stage 6.5: drive STA on top of the chip-net topology. Smoke test —
# create_clock on clk_top chip-net (Stage 5 findNetAllScopes chip-aware)
# and call report_checks. Real cross-chiplet timing paths need
# chiplet-internal block access (Stage 7); for now we just confirm STA
# doesn't crash on the chip-aware accessors.
create_clock -name clk -period 1.0 \
  [get_pins -of_objects [get_nets clk_top]]
# True flop-to-flop constrained paths across the chip-net need dual-
# vertex hierarchical pins (load + driver per chip-bump). That's Stage 7.
# For now -unconstrained reports the longest data chains STA can trace,
# which now include real Liberty delays for inv/buf in each chiplet.
report_checks -unconstrained -path_delay max -group_path_count 4

exit_summary

