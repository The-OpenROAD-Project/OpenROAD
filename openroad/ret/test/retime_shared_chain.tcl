# The graph walk ends at a net with two drivers.  Here 25 stages of two
# registers follow each other; both registers of a stage drive one net and
# both registers of the next stage read it.  Walked on from every driver,
# the last net would be reached 2^25 times.  Nothing behind such a net
# moves.
source "helpers.tcl"
set netlist [make_result_file retime_shared_chain.v]
set stream [open $netlist w]
puts $stream "module top (a, b, clk, y);"
puts $stream "  input a;\n  input b;\n  input clk;\n  output y;"
puts $stream "  wire n1, n2, n3, n4, q2;"
for { set i 0 } { $i <= 25 } { incr i } {
  puts $stream "  wire s$i;"
}
puts $stream "  AND2_X1 g0 (.A1(a), .A2(b), .ZN(s0));"
for { set i 1 } { $i <= 25 } { incr i } {
  set input s[expr { $i - 1 }]
  puts $stream "  DFF_X1 ra$i (.D($input), .CK(clk), .Q(s$i));"
  puts $stream "  DFF_X1 rb$i (.D($input), .CK(clk), .Q(s$i));"
}
puts $stream "  INV_X1 g1 (.A(s25), .ZN(n1));"
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
