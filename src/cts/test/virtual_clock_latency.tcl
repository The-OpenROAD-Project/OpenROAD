source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def "16sinks.def"

create_clock -period 5 clk
create_clock -name vclk -period 5
set_clock_latency 0.2 [get_clocks vclk]

set_wire_rc -clock -layer metal3

set_cts_config -wire_unit 20 \
  -apply_ndr root_only \
  -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3

clock_tree_synthesis

set sdc_file [make_result_file virtual_clock_latency.sdc]
write_sdc -no_timestamp $sdc_file

set stream [open $sdc_file r]
set sdc_text [read $stream]
close $stream

set vclk_latency_found \
  [regexp {set_clock_latency 0\.2000 \[get_clocks \{vclk\}\]} $sdc_text]
set vclk_propagated_found \
  [regexp {set_propagated_clock \[get_clocks \{vclk\}\]} $sdc_text]

# vclk latency should be preserved
puts "vclk latency preserved: $vclk_latency_found"

# vclk cannot be propagated
puts "vclk propagated written: $vclk_propagated_found"
