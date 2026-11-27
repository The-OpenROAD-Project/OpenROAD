# The H-tree stops when its largest sink region stops getting shorter
# (CTS-0054).
#
# ff0 and ff1 lie 1490 um apart along the bottom edge, ff2 alone at the
# top right. Level 1 splits the sinks left and right, which leaves ff0
# and ff1 together; level 2 splits top and bottom, and both of them are
# nearer the bottom branch, so they stay together. The largest sink
# region is still the 1490 um span of ff0 and ff1, longer than the max
# wire length, and the H-tree must stop at level 2 instead of
# subdividing to its maximum depth.
source "helpers.tcl"
source "clock_tree_checks.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def "htree_stop_no_improvement.def"

create_clock -period 5 clk
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5

set_cts_config -wire_unit 20 \
  -clustering_unbalance_ratio 1.0 \
  -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3

tee -quiet -variable cts_log { clock_tree_synthesis }

check "the H-tree stops because the sink regions did not get shorter" \
  { regexp {CTS-0054\]\s+Stop criterion found} $cts_log } 1
check "the largest sink region is the span of ff0 and ff1" \
  { regexp {CTS-0054\][^\n]*hpwl \((\d+)\.\d+ um\)} $cts_log -> hpwl
    set hpwl } 1490
check "the span is longer than the max wire length" \
  { expr { 1490 > [rsz::find_max_wire_length] * 1e6 } } 1
check "the H-tree stops at level 2" \
  { regexp {CTS-0017\]\s+Max level of the clock tree: (\d+)} $cts_log -> levels
    set levels } 2

set block [ord::get_db_block]
check "every flop is clocked from clk" \
  { unclocked_count [clock_pins $block ff* CK] clk } 0

exit_summary
