# A run of more than 256 registers with no gate in between is left alone:
# the graph walk goes one level deeper and every edge holds one more
# register for each of them.  Here 300 registers stand in front of the
# failing cone of retime_basic: the first 256 are fenced, the walk ends
# there, and the gates behind the run are never crossed.
source "helpers.tcl"
set netlist [make_result_file retime_long_chain.v]
set stream [open $netlist w]
puts $stream "module top (a, b, clk, y);"
puts $stream "  input a;\n  input b;\n  input clk;\n  output y;"
puts $stream "  wire n1, n2, n3, n4, q2;"
for { set i 1 } { $i <= 301 } { incr i } {
  puts $stream "  wire d$i;"
}
puts $stream "  AND2_X1 g0 (.A1(a), .A2(b), .ZN(d1));"
for { set i 1 } { $i <= 300 } { incr i } {
  puts $stream "  DFF_X1 s$i (.D(d$i), .CK(clk), .Q(d[expr { $i + 1 }]));"
}
puts $stream "  INV_X1 g1 (.A(d301), .ZN(n1));"
puts $stream "  INV_X1 g2 (.A(n1), .ZN(n2));"
puts $stream "  INV_X1 g3 (.A(n2), .ZN(n3));"
puts $stream "  INV_X1 g4 (.A(n3), .ZN(n4));"
puts $stream "  DFF_X1 r2 (.D(n4), .CK(clk), .Q(q2));"
puts $stream "  BUF_X1 g5 (.A(q2), .Z(y));"
puts $stream "endmodule"
close $stream
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog $netlist
link_design top
create_clock -period 0.1 clk

retime
