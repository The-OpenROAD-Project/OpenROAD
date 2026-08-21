// TOP: top
// TECH: nangate45
// TARGETS: part_select_port, depth_2
// CLUE: Part-selects that shift at each hierarchy level: top x[9:2] -> mid [7:0]; mid p[5:2] -> leaf [3:0]; leaf output re-embedded at q[5:2].
module leaf (a, y);
  input [3:0] a;
  output [3:0] y;
  INV_X1 g0 (.A(a[3]), .ZN(y[3]));
  INV_X1 g1 (.A(a[2]), .ZN(y[2]));
  INV_X1 g2 (.A(a[1]), .ZN(y[1]));
  INV_X1 g3 (.A(a[0]), .ZN(y[0]));
endmodule
module mid (p, q);
  input [7:0] p;
  output [7:0] q;
  leaf l (.a(p[5:2]), .y(q[5:2]));
  BUF_X1 c0 (.A(p[0]), .Z(q[0]));
  BUF_X1 c1 (.A(p[1]), .Z(q[1]));
  BUF_X1 c6 (.A(p[6]), .Z(q[6]));
  BUF_X1 c7 (.A(p[7]), .Z(q[7]));
endmodule
module top (x, z);
  input [9:0] x;
  output [7:0] z;
  mid m (.p(x[9:2]), .q(z));
endmodule
