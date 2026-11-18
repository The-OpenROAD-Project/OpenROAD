source "helpers.tcl"
source "clock_tree_checks.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def "max_wl_two_sinks.def"

create_clock -period 5 clk
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5

# High unbalance ratio allows both sinks to cluster into the same
# branch, reproducing the scenario where HPWL never decreases and
# the H-tree loop would previously run to max depth.
set_cts_config -wire_unit 20 \
  -clustering_unbalance_ratio 1.0 \
  -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3

tee -quiet -variable cts_log { clock_tree_synthesis }
report_cts

# The sinks lie on a horizontal line. Level 1 splits it in two; level 2
# splits vertically, which leaves sink regions of zero height, and the
# H-tree must stop there on one of its stop criteria (the regions stopped
# getting shorter, CTS-0054, or are shorter than the max wire length,
# CTS-0038) instead of subdividing to its maximum depth.
check "the H-tree stops on a stop criterion" \
  { regexp {CTS-00(54|38)\]\s+Stop criterion found} $cts_log } 1
check "the H-tree stops at level 2" \
  { regexp {CTS-0017\]\s+Max level of the clock tree: (\d+)} $cts_log -> levels
    set levels } 2

set block [ord::get_db_block]
check "every flop is clocked from clk" \
  { unclocked_count [clock_pins $block ff* CK] clk } 0

exit_summary
