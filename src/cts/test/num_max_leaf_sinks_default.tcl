# Default-safety check for the -num_max_leaf_sinks knob.
#
# When the option is NOT passed, the effective threshold stays at the
# existing default (15) and CTS must produce the identical baseline tree
# (3 buffers, H-tree level 1 on 16sinks.def) -- i.e. the new knob is
# additive and changes nothing unless explicitly opted into.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def "16sinks.def"

create_clock -period 5 clk

set_wire_rc -clock -layer metal3

set_cts_config -wire_unit 20 \
  -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3

clock_tree_synthesis

puts "Effective num_max_leaf_sinks = [cts::get_num_max_leaf_sinks]"

report_clock_tree
