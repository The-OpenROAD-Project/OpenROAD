# The clock tree watermark round-trips: the parity the embedder sets is the
# parity verification reads back.
#
# The mark is the parity of a leaf clock buffer's sequential fanout, set by
# moving one flip-flop's clock pin to the neighbouring buffer.  Both sides have
# to agree on what a leaf clock buffer is and what counts as a sink, so the
# round trip is the thing worth pinning: a definition that drifted between
# embed and verify would turn a valid watermark into a failed one.
#
# gcd has 34 sinks and TritonCTS gives it four leaf buffers, so there are two
# pairs to mark.  That is small, but the outcome is fully determined by the
# key -- nothing here is sampled.
#
# cts_strict.tcl is the other half: it runs the same embed with a skew budget
# nothing can meet, where the claims must be written and must not hold.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_placed.def
create_clock -name core_clock -period 2.0 [get_ports clk]
set_wire_rc -clock -layer metal5
clock_tree_synthesis -buf_list CLKBUF_X3 -root_buf CLKBUF_X3 -sink_clustering_enable

# Moving a sink is only allowed if it does not cost skew, so there has to be a
# propagated clock to measure skew on.
set_propagated_clock [all_clocks]
estimate_parasitics -placement

set key 0011223344556677889900aabbccddeeff00112233445566778899aabbccddee
set claims [make_result_file cts.csv]

set committed [cts_watermark -key_hex $key -claims_file $claims]
puts "committed $committed pairs"
check "the clock tree yields its two pairs" { set committed } 2
check "every claimed parity holds" { verify_watermark -cts_claims $claims } 1

exit_summary
