// TOP: top
// TECH: nangate45
// TARGETS: neg_bounds, whole_bus_connect, depth_1
// CLUE: Bracket: child port range ENTIRELY negative [-5:-2]; hier path should disconnect all four bits if the negative-index bug bites every bit.
module sub (a, y);
  input [-5:-2] a;
  output [-5:-2] y;
  INV_X1 g0 (.A(a[-5]), .ZN(y[-5]));
  INV_X1 g1 (.A(a[-4]), .ZN(y[-4]));
  INV_X1 g2 (.A(a[-3]), .ZN(y[-3]));
  INV_X1 g3 (.A(a[-2]), .ZN(y[-2]));
endmodule
module top (x, z);
  input [3:0] x;
  output [3:0] z;
  sub s (.a(x), .y(z));
endmodule
