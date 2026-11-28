// rq is a QDFF_X8, whose LEF has an input, EXTRA, that its liberty cell
// lacks: rq never moves, so the inverter it drives is never crossed.  r1 in
// front of the same cone is a DFF_X1 and slides forward.
module top_pins (a, b, x, clk, y, z);
  input a;
  input b;
  input x;
  input clk;
  output y;
  output z;
  wire n0, q1, n1, n2, n3, n4, q2;
  wire m0, qq, m1, m2, m3, m4, q3;

  AND2_X1 g0 (.A1(a), .A2(b), .ZN(n0));
  DFF_X1 r1 (.D(n0), .CK(clk), .Q(q1));
  INV_X1 g1 (.A(q1), .ZN(n1));
  INV_X1 g2 (.A(n1), .ZN(n2));
  INV_X1 g3 (.A(n2), .ZN(n3));
  INV_X1 g4 (.A(n3), .ZN(n4));
  DFF_X1 r2 (.D(n4), .CK(clk), .Q(q2));
  BUF_X1 g5 (.A(q2), .Z(y));

  OR2_X1 g6 (.A1(a), .A2(b), .ZN(m0));
  QDFF_X8 rq (.D(m0), .CK(clk), .Q(qq));
  INV_X1 g7 (.A(qq), .ZN(m1));
  INV_X1 g8 (.A(m1), .ZN(m2));
  INV_X1 g9 (.A(m2), .ZN(m3));
  INV_X1 g10 (.A(m3), .ZN(m4));
  DFF_X1 r3 (.D(m4), .CK(clk), .Q(q3));
  BUF_X1 g11 (.A(q3), .Z(z));
endmodule
