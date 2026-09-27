// TOP: top
// TECH: nangate45
// TARGETS: part_select_port, output_side, depth_1
// CLUE: Child OUTPUT connected to interior part-select z[5:2] of a wider top output; other z bits driven by buffers.
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
  output [7:0] z;
  sub s (.a(x[3:0]), .y(z[5:2]));
  BUF_X1 b0 (.A(x[4]), .Z(z[0]));
  BUF_X1 b1 (.A(x[5]), .Z(z[1]));
  BUF_X1 b6 (.A(x[6]), .Z(z[6]));
  BUF_X1 b7 (.A(x[7]), .Z(z[7]));
endmodule
