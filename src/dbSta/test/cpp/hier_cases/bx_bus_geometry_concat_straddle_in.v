// TOP: top
// TECH: nangate45
// TARGETS: concat_port, two_nets, depth_1
// CLUE: Concat {xh,xl} straddling two separate 2-bit nets into one 4-bit child input.
module sub (a, y);
  input [3:0] a;
  output [3:0] y;
  INV_X1 g0 (.A(a[3]), .ZN(y[3]));
  INV_X1 g1 (.A(a[2]), .ZN(y[2]));
  INV_X1 g2 (.A(a[1]), .ZN(y[1]));
  INV_X1 g3 (.A(a[0]), .ZN(y[0]));
endmodule
module top (xh, xl, z);
  input [1:0] xh;
  input [1:0] xl;
  output [3:0] z;
  sub s (.a({xh,xl}), .y(z));
endmodule
