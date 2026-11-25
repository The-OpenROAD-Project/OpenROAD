// TOP: top
// TECH: nangate45
// TARGETS: asc_range, whole_bus_connect, depth_1
// CLUE: Only child INPUT is ascending [0:3]; output side normal. Brackets asc_child_both.
module sub (a, y);
  input [0:3] a;
  output [3:0] y;
  INV_X1 g0 (.A(a[0]), .ZN(y[3]));
  INV_X1 g1 (.A(a[1]), .ZN(y[2]));
  INV_X1 g2 (.A(a[2]), .ZN(y[1]));
  INV_X1 g3 (.A(a[3]), .ZN(y[0]));
endmodule
module top (x, z);
  input [3:0] x;
  output [3:0] z;
  sub s (.a(x), .y(z));
endmodule
