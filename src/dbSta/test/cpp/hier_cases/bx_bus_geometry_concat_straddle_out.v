// TOP: top
// TECH: nangate45
// TARGETS: concat_port, two_nets, output_side, depth_1
// CLUE: Child OUTPUT split across two nets via lvalue concat {zh,zl}.
module sub (a, y);
  input [3:0] a;
  output [3:0] y;
  INV_X1 g0 (.A(a[3]), .ZN(y[3]));
  INV_X1 g1 (.A(a[2]), .ZN(y[2]));
  INV_X1 g2 (.A(a[1]), .ZN(y[1]));
  INV_X1 g3 (.A(a[0]), .ZN(y[0]));
endmodule
module top (x, zh, zl);
  input [3:0] x;
  output [1:0] zh;
  output [1:0] zl;
  sub s (.a(x), .y({zh,zl}));
endmodule
