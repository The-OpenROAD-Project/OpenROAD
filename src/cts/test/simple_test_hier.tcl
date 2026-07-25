# Test CTS gated clock tree synthesis on hierarchical design with 16 sinks
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
#read_lef dummy_pads.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog simple_test_hier.v
link test_16_sinks -hier
read_def -floorplan_initialize simple_test_hier.def

create_clock -period 5 clk

set_wire_rc -clock -layer metal3

set_cts_config -wire_unit 20 -apply_ndr root_only \
  -root_buf CLKBUF_X3 -buf_list CLKBUF_X3

clock_tree_synthesis

set verilog_file [make_result_file simple_test_hier_out.v]
write_verilog $verilog_file
diff_files simple_test_hier_out.vok $verilog_file
