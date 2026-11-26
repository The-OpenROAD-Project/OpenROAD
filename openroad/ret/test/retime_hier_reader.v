// core: r1 and r2 feed one NAND and a long cone, all inside core.  The Q net
// of r1 also leaves core through port t and ends on the D pin of a register
// in another module whose output nothing reads.
module core (a, b, rn, ck, y, t);
  input a;
  input b;
  input rn;
  input ck;
  output y;
  output t;
  wire d1, d2, q2, n1, n2, n3, n4, n5, n6, d3, q3;

  AND2_X1 g0 (.A1(a), .A2(rn), .ZN(d1));
  AND2_X1 g1 (.A1(b), .A2(rn), .ZN(d2));
  DFF_X1 r1 (.D(d1), .CK(ck), .Q(t));
  DFF_X1 r2 (.D(d2), .CK(ck), .Q(q2));
  NAND2_X1 g2 (.A1(t), .A2(q2), .ZN(n1));
  INV_X1 g3 (.A(n1), .ZN(n2));
  INV_X1 g4 (.A(n2), .ZN(n3));
  INV_X1 g5 (.A(n3), .ZN(n4));
  INV_X1 g6 (.A(n4), .ZN(n5));
  INV_X1 g7 (.A(n5), .ZN(n6));
  AND2_X1 g8 (.A1(n6), .A2(rn), .ZN(d3));
  DFF_X1 r3 (.D(d3), .CK(ck), .Q(q3));
  BUF_X1 g9 (.A(q3), .Z(y));
endmodule

module spy (i, ck);
  input i;
  input ck;

  DFF_X1 rx (.D(i), .CK(ck));
endmodule

module top (a, b, rst_n, clk, y);
  input a;
  input b;
  input rst_n;
  input clk;
  output y;
  wire tq;

  core u_core (.a(a), .b(b), .rn(rst_n), .ck(clk), .y(y), .t(tq));
  spy u_spy (.i(tq), .ck(clk));
endmodule
