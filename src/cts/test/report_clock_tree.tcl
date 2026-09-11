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

# New additive, read-only QoR report.  Reports sinks, buffers, levels,
# clock-net wire length, achieved insertion delay (latency) min/max/avg,
# clock skew, and an estimate of clock switching power.
report_clock_tree -power
