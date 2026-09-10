// TOP: top
// TECH: nangate45
// TARGETS: asc_range, nonzero_base, whole_bus_connect, depth_1
// CLUE: Child ports ascending AND non-zero-based [5:8] vs parent [3:0]; combines direction flip with base offset.
module sub (a, y);
  input [5:8] a;
  output [5:8] y;
  INV_X1 g0 (.A(a[5]), .ZN(y[5]));
  INV_X1 g1 (.A(a[6]), .ZN(y[6]));
  INV_X1 g2 (.A(a[7]), .ZN(y[7]));
  INV_X1 g3 (.A(a[8]), .ZN(y[8]));
endmodule
module top (x, z);
  input [3:0] x;
  output [3:0] z;
  sub s (.a(x), .y(z));
endmodule
