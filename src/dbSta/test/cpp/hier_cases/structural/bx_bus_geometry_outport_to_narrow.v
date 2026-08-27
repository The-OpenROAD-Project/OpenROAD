// TOP: top
// TECH: nangate45
// TARGETS: width_mismatch, output_side, depth_1
// CLUE: Child output [3:0] connected to a 2-bit net; upper output bits truncated at the boundary.
module sub (a, y);
  input [3:0] a;
  output [3:0] y;
  INV_X1 g0 (.A(a[3]), .ZN(y[3]));
  INV_X1 g1 (.A(a[2]), .ZN(y[2]));
  INV_X1 g2 (.A(a[1]), .ZN(y[1]));
  INV_X1 g3 (.A(a[0]), .ZN(y[0]));
endmodule
module top (x, z);
  input [3:0] x;
  output [1:0] z;
  wire [1:0] w;
  sub s (.a(x), .y(w));
  BUF_X1 b0 (.A(w[0]), .Z(z[0]));
  BUF_X1 b1 (.A(w[1]), .Z(z[1]));
endmodule
