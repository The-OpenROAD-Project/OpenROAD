// TOP: top
// TECH: nangate45
// TARGETS: escaped_port, bus_port, char_slash, depth_2
// CLUE: submodule BUS port named \b/c  (wire [3:0]) with bit-selects
// \b/c [i] inside; slash in a bus port name at depth 2.
module subb (input [3:0] \b/c , output z);
  wire t0, t1;
  XOR2_X1 g1 (.A(\b/c [0]), .B(\b/c [1]), .Z(t0));
  XOR2_X1 g2 (.A(\b/c [2]), .B(\b/c [3]), .Z(t1));
  XOR2_X1 g3 (.A(t0), .B(t1), .Z(z));
endmodule
module top (input [3:0] a, output z);
  subb u1 (.\b/c (a), .z(z));
endmodule
