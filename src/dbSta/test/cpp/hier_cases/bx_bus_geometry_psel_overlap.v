// TOP: top
// TECH: nangate45
// TARGETS: part_select_port, overlapping_slices, depth_1
// CLUE: Two children take OVERLAPPING part-selects of the same bus (x[4:1] and
// x[3:0]); shared bits x[3:1] land at different positions in each port.
module sub (a, y);
  input [3:0] a;
  output [3:0] y;
  INV_X1   g0 (.A(a[0]), .ZN(y[0]));
  BUF_X1   g1 (.A(a[1]), .Z(y[1]));
  NAND2_X1 g2 (.A1(a[2]), .A2(a[0]), .ZN(y[2]));
  NOR2_X1  g3 (.A1(a[3]), .A2(a[1]), .ZN(y[3]));
endmodule
module top (x, p, q);
  input [4:0] x;
  output [3:0] p;
  output [3:0] q;
  sub u0 (.a(x[4:1]), .y(p));
  sub u1 (.a(x[3:0]), .y(q));
endmodule
