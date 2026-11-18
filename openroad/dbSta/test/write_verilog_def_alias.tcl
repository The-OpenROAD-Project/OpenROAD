# Preserve differently named input nets when writing a DEF-based design.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def ../../grt/test/upper_layer_net.def

set verilog_file [make_result_file write_verilog_def_alias.v]
write_verilog $verilog_file
set stream [open $verilog_file r]
set text [read $stream]
close $stream
foreach bit {0 1 2} {
  set expected [format {assign net%d = req_msg[%d];} $bit $bit]
  if { [string first $expected $text] < 0 } {
    error "Missing input alias: $expected"
  }
}
puts "pass"
