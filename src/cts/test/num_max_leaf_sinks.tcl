# Opt-in path for the -num_max_leaf_sinks knob.
#
# The H-tree stop criterion keeps adding levels until the number of sinks
# per sub-region drops below num_max_leaf_sinks.  Lowering it below the
# default (15) forces a deeper H-tree with more branch buffers and a
# tighter (more uniform) latency balance.  On 16sinks.def the default
# stops at level 1 (3 buffers); -num_max_leaf_sinks 4 grows the tree to
# level 3 (9 buffers).  See num_max_leaf_sinks_default.tcl for the proof
# that the default (knob unset) reproduces the baseline tree exactly.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def "16sinks.def"

create_clock -period 5 clk

set_wire_rc -clock -layer metal3

set_cts_config -wire_unit 20 \
  -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3

clock_tree_synthesis -num_max_leaf_sinks 4

puts "Effective num_max_leaf_sinks = [cts::get_num_max_leaf_sinks]"

report_clock_tree
