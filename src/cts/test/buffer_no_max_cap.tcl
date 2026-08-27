# Buffer inference fails when liberty buffers have no max capacitance
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ_no_max_cap.lib
read_def "16sinks.def"

create_clock -period 5 clk
set_wire_rc -clock -layer metal3

set_cts_config -wire_unit 20 \
  -buf_list "CLKBUF_X3 CLKBUF_X2 CLKBUF_X1"

catch {
  clock_tree_synthesis
} error

puts $error
