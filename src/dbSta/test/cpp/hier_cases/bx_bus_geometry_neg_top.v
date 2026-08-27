// TOP: top
// TECH: nangate45
// TARGETS: negative_bounds, top_port_m2to1
// CLUE: Top-level ports declared [-2:1]. Emitted top port shape and bit
// correspondence with the [3:0] child must survive.
module sub (p, q);
  input [3:0] p;
  output [3:0] q;
  INV_X1 g0 (.A(p[0]), .ZN(q[0]));
  BUF_X1 g1 (.A(p[1]), .Z(q[1]));
  NAND2_X1 g2 (.A1(p[2]), .A2(p[3]), .ZN(q[2]));
  XOR2_X1 g3 (.A(p[3]), .B(p[0]), .Z(q[3]));
endmodule
module top (in, out);
  input [-2:1] in;
  output [-2:1] out;
  sub u0 (.p(in), .q(out));
endmodule
