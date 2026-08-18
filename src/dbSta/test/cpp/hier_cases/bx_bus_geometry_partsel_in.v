// TOP: top
// TECH: nangate45
// TARGETS: part_select_port, depth_1
// CLUE: Child 4-bit input connected to interior part-select x[5:2] of a wider top input; x[7:6],x[1:0] unused.
module sub (a, y);
  input [3:0] a;
  output [3:0] y;
  INV_X1 g0 (.A(a[3]), .ZN(y[3]));
  INV_X1 g1 (.A(a[2]), .ZN(y[2]));
  INV_X1 g2 (.A(a[1]), .ZN(y[1]));
  INV_X1 g3 (.A(a[0]), .ZN(y[0]));
endmodule
module top (x, z);
  input [7:0] x;
  output [3:0] z;
  sub s (.a(x[5:2]), .y(z));
endmodule
