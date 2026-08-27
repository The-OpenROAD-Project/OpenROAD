// TOP: top
// TECH: nangate45
// TARGETS: neg_bounds, asc_range, depth_1
// CLUE: Child ports [-2:1] (negative bound, ascending) - legal per LRM; probes reader index handling.
module sub (a, y);
  input [-2:1] a;
  output [-2:1] y;
  INV_X1 g0 (.A(a[-2]), .ZN(y[-2]));
  INV_X1 g1 (.A(a[-1]), .ZN(y[-1]));
  INV_X1 g2 (.A(a[0]), .ZN(y[0]));
  INV_X1 g3 (.A(a[1]), .ZN(y[1]));
endmodule
module top (x, z);
  input [3:0] x;
  output [3:0] z;
  sub s (.a(x), .y(z));
endmodule
