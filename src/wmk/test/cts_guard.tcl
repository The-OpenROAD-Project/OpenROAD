# Companion to cts.tcl: a key whose mark the clock cannot afford.
#
# Two things are being checked, and they are the two that decide whether the
# extraction rate means anything.
#
# The skew guard has to be live.  Moving a sink changes the load on two
# buffers, and two of this key's three pairs cannot absorb it, so those pairs
# have to be turned away.
#
# The budget is set to zero here, which demands that a move cost no skew at
# all.  The default is 20 ps, deliberately: a sink move almost always costs a
# little, and a zero budget turns the stage off wherever the clock is tight.
# Zero is what makes the guard observable on a tree this small.  If the guard
# were dead -- if skew came back as a
# constant, say -- nothing would ever be rejected and the watermark would be
# free to damage the clock it is marking.
#
# And the pair the guard turned away has to be claimed anyway.  Dropping it
# would let the embedder choose its evidence after seeing the design, and an
# extraction rate picked that way is one on every design, marked or not.  So
# the rate here has to come out low, not high: the claims are honest about
# having failed, and the ownership verdict follows the truth.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_placed.def
create_clock -name core_clock -period 2.0 [get_ports clk]
set_wire_rc -clock -layer metal5
clock_tree_synthesis -buf_list CLKBUF_X3 -root_buf CLKBUF_X3 -sink_clustering_enable

set_propagated_clock [all_clocks]
estimate_parasitics -placement

set key 0000000000000000000000000000000000000000000000000000000000000003
set claims [make_result_file cts_guard.csv]

set committed [cts_watermark -key_hex $key -claims_file $claims \
  -skew_margin_ns 0]
puts "committed $committed pairs"
check "the pairs the guard turned away are still claimed" { set committed } 3
check "and the claim does not hold" { verify_watermark -cts_claims $claims -min_stages 1 } 0

exit_summary
