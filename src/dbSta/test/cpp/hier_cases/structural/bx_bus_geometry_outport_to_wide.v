// TOP: top
// TECH: nangate45
// TARGETS: width_mismatch, output_side, depth_1
// CLUE: Child output [1:0] connected to 4-bit top output; z[3:2] implicitly extended or left undriven depending on tool.
module sub (a, y);
  input [1:0] a;
  output [1:0] y;
  INV_X1 g0 (.A(a[1]), .ZN(y[1]));
  INV_X1 g1 (.A(a[0]), .ZN(y[0]));
endmodule
module top (x, z);
  input [1:0] x;
  output [3:0] z;
  sub s (.a(x), .y(z));
endmodule
