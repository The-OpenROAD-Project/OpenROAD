// TOP: top
// TECH: nangate45
// TARGETS: neg_bounds, part_select_port, depth_1
// CLUE: Part-select x[-2:1] spanning zero on a [-4:3] top input; negative indices inside a part-select.
module sub (a, y);
  input [3:0] a;
  output [3:0] y;
  INV_X1 g0 (.A(a[3]), .ZN(y[3]));
  INV_X1 g1 (.A(a[2]), .ZN(y[2]));
  INV_X1 g2 (.A(a[1]), .ZN(y[1]));
  INV_X1 g3 (.A(a[0]), .ZN(y[0]));
endmodule
module top (x, z);
  input [-4:3] x;
  output [3:0] z;
  sub s (.a(x[-2:1]), .y(z));
endmodule
