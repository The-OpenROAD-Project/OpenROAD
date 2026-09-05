# The clock tree mark respects what the library says a buffer can drive.
#
# Moving a sink onto a leaf buffer adds load to it.  How much load is too much
# is the library's answer, not a number this module should carry: the limits
# come from the liberty cell, so the same command behaves differently on a
# technology with different buffers, which is the point.
#
# The headroom asked for here is absurd -- 99% of both limits left unused -- so
# every move has to be turned away.  At the default of 20% none of them are, on
# this design or on a routed NanGate45 aes, which is the healthy case: it says
# the marks are nowhere near what the buffers can take.  Testing at the default
# would therefore test nothing, since a check that never fires and a check that
# is not wired up look identical.
#
# The pairs are still claimed, and the extraction rate still tells the truth
# about them, for the same reason as in cts_guard.tcl.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_placed.def
create_clock -name core_clock -period 2.0 [get_ports clk]
set_wire_rc -clock -layer metal5
clock_tree_synthesis -buf_list CLKBUF_X3 -root_buf CLKBUF_X3 -sink_clustering_enable

set_propagated_clock [all_clocks]
estimate_parasitics -placement

set key 0000000000000000000000000000000000000000000000000000000000000004
set claims [make_result_file cts_drive.csv]

set committed [cts_watermark -key_hex $key -claims_file $claims \
  -slew_headroom_frac 0.99 -cap_headroom_frac 0.99]
puts "committed $committed pairs"
check "the pairs the library turned away are still claimed" { set committed } 3
check "and none of the claims hold" { verify_watermark -cts_claims $claims -min_stages 1 } 0

exit_summary
