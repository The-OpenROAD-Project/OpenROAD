// TOP: top
// TECH: nangate45
// TARGETS: width_mismatch, port_narrower, depth_1
// CLUE: Child input port [1:0] connected to a 4-bit expression; LRM truncation keeps LSBs. Probe reader.
module sub (a, y);
  input [1:0] a;
  output [1:0] y;
  INV_X1 g0 (.A(a[1]), .ZN(y[1]));
  INV_X1 g1 (.A(a[0]), .ZN(y[0]));
endmodule
module top (x, z);
  input [3:0] x;
  output [1:0] z;
  sub s (.a(x), .y(z));
endmodule
