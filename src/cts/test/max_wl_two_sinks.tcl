source "helpers.tcl"
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

clock_tree_synthesis
report_cts
