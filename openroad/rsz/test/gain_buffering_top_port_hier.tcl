# repair_design -pre_placement gain-buffers a top-level input port that
# drives more loads than max_fanout inside a kept module. In a hierarchical
# netlist the port's term net is the top module's dbModNet; early sizing
# must hand gain buffering the port's flat dbNet instead.
source "helpers.tcl"

set test_name gain_buffering_top_port_hier
set fanout 64

# top has an input port d that drives $fanout flops inside the kept module
# sub, with no buffer in between.
set verilog_file [make_result_file ${test_name}.v]
set stream [open $verilog_file w]
puts $stream "module sub (clk, d);"
puts $stream "  input clk;"
puts $stream "  input d;"
for { set i 0 } { $i < $fanout } { incr i } {
  puts $stream "  DFF_X1 ff$i (.CK(clk), .D(d));"
}
puts $stream "endmodule"
puts $stream "module top (clk, d);"
puts $stream "  input clk;"
puts $stream "  input d;"
puts $stream "  sub u_sub (.clk(clk), .d(d));"
puts $stream "endmodule"
close $stream

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog $verilog_file
link_design top -hier

create_clock -name clk -period 1.0 [get_ports clk]
set_input_delay 0.5 -clock clk [get_ports d]
set_max_fanout 32 [current_design]

proc port_net_fanout { port } {
  set net [[ord::get_db_block] findNet $port]
  return [llength [$net getITerms]]
}

check "port d drives all the flops" { port_net_fanout d } $fanout

set failed [catch { repair_design -pre_placement } message]
if { $failed } {
  puts $message
}
check "repair_design -pre_placement returns" { set failed } 0
check "port d is buffered to at most max_fanout loads" \
  { expr { [port_net_fanout d] <= 32 } } 1
check "port d still reaches the flops through buffers" \
  { expr { [port_net_fanout d] > 0 } } 1

exit_summary
