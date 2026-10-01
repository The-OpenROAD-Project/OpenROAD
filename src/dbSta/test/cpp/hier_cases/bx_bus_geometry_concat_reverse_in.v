// TOP: top
// TECH: nangate45
// TARGETS: concat_port, bit_reversal, depth_1
// CLUE: Bit reversal via concat {x[0],x[1],x[2],x[3]} on a child input; equivalence depends on exact bit order survival.
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
  output [3:0] z;
  sub s (.a({x[0],x[1],x[2],x[3]}), .y(z));
endmodule
