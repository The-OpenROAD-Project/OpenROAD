// TOP: top
// TECH: nangate45
// TARGETS: width_mismatch, port_wider, depth_1
// CLUE: Child input port [3:0] connected to a 2-bit expression; LRM implicit zero-extension. Legal with warning - probe reader.
module sub (a, y);
  input [3:0] a;
  output [3:0] y;
  INV_X1 g0 (.A(a[3]), .ZN(y[3]));
  INV_X1 g1 (.A(a[2]), .ZN(y[2]));
  INV_X1 g2 (.A(a[1]), .ZN(y[1]));
  INV_X1 g3 (.A(a[0]), .ZN(y[0]));
endmodule
module top (x, z);
  input [1:0] x;
  output [3:0] z;
  sub s (.a(x), .y(z));
endmodule
