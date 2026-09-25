// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_output_dangling
// CLUE: entire 4-bit sub output bus q lands in wire qw that nothing consumes.
//       All four internal drivers of sub become unobservable dead logic.
module sub (a, q);
  input a;
  output [3:0] q;
  INV_X1 g0 (.A(a), .ZN(q[0]));
  BUF_X1 g1 (.A(a), .Z(q[1]));
  INV_X1 g2 (.A(a), .ZN(q[2]));
  BUF_X1 g3 (.A(a), .Z(q[3]));
endmodule

module top (x, y);
  input x;
  output y;
  wire [3:0] qw;
  INV_X1 u1 (.A(x), .ZN(y));
  sub u0 (.a(x), .q(qw));
endmodule
