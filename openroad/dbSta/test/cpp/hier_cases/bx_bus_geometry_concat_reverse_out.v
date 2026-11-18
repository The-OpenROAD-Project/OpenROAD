// TOP: top
// TECH: nangate45
// TARGETS: concat_port, bit_reversal, output_side, depth_1
// CLUE: Child OUTPUT connected to reversed lvalue concat {z[0],z[1],z[2],z[3]}.
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
  sub s (.a(x), .y({z[0],z[1],z[2],z[3]}));
endmodule
