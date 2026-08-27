// TOP: top
// TECH: nangate45
// TARGETS: bit_local_and_through, depth_1
// CLUE: Bus bit x[2] consumed by a top-level gate while the WHOLE bus (including x[2]) also feeds through the child.
module sub (a, y);
  input [3:0] a;
  output [3:0] y;
  INV_X1 g0 (.A(a[3]), .ZN(y[3]));
  INV_X1 g1 (.A(a[2]), .ZN(y[2]));
  INV_X1 g2 (.A(a[1]), .ZN(y[1]));
  INV_X1 g3 (.A(a[0]), .ZN(y[0]));
endmodule
module top (x, z, zl);
  input [3:0] x;
  output [3:0] z;
  output zl;
  sub s (.a(x), .y(z));
  INV_X1 g (.A(x[2]), .ZN(zl));
endmodule
