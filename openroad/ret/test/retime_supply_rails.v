// Four failing cones on one clock, each with a register in front of four
// inverters.  rp is rebuilt as the base cell, which has other supply pin
// names.  ra and rb reach one gate, gm, from two power nets, each through
// two inverters.  rs has its two power pins on two nets, rq one of them open.
module top (a, b, c, d, e, f, g, h, clk, y1, y2, y3, y4);
  input a;
  input b;
  input c;
  input d;
  input e;
  input f;
  input g;
  input h;
  input clk;
  output y1;
  output y2;
  output y3;
  output y4;
  wire n0, q1, n1, n2, n3, n4, q2;
  wire da, db, qa, qb, a1, a2, b1, b2, m0, m1, m2, m3, m4, q3;
  wire k0, qs, k1, k2, k3, k4, q4;
  wire p0, qq, p1, p2, p3, p4, q5;

  AND2_X1 g0 (.A1(a), .A2(b), .ZN(n0));
  DFF_X1 rp (.D(n0), .CK(clk), .Q(q1));
  INV_X1 g1 (.A(q1), .ZN(n1));
  INV_X1 g2 (.A(n1), .ZN(n2));
  INV_X1 g3 (.A(n2), .ZN(n3));
  INV_X1 g4 (.A(n3), .ZN(n4));
  DFF_X1 r2 (.D(n4), .CK(clk), .Q(q2));
  BUF_X1 g5 (.A(q2), .Z(y1));

  AND2_X1 ha (.A1(c), .A2(d), .ZN(da));
  OR2_X1 hb (.A1(c), .A2(d), .ZN(db));
  DFF_X1 ra (.D(da), .CK(clk), .Q(qa));
  DFF_X1 rb (.D(db), .CK(clk), .Q(qb));
  INV_X1 ia1 (.A(qa), .ZN(a1));
  INV_X1 ia2 (.A(a1), .ZN(a2));
  INV_X1 ib1 (.A(qb), .ZN(b1));
  INV_X1 ib2 (.A(b1), .ZN(b2));
  AND2_X1 gm (.A1(a2), .A2(b2), .ZN(m0));
  INV_X1 h1 (.A(m0), .ZN(m1));
  INV_X1 h2 (.A(m1), .ZN(m2));
  INV_X1 h3 (.A(m2), .ZN(m3));
  INV_X1 h4 (.A(m3), .ZN(m4));
  DFF_X1 r3 (.D(m4), .CK(clk), .Q(q3));
  BUF_X1 h5 (.A(q3), .Z(y2));

  AND2_X1 j0 (.A1(e), .A2(f), .ZN(k0));
  DFFP_X1 rs (.D(k0), .CK(clk), .Q(qs));
  INV_X1 j1 (.A(qs), .ZN(k1));
  INV_X1 j2 (.A(k1), .ZN(k2));
  INV_X1 j3 (.A(k2), .ZN(k3));
  INV_X1 j4 (.A(k3), .ZN(k4));
  DFF_X1 r4 (.D(k4), .CK(clk), .Q(q4));
  BUF_X1 j5 (.A(q4), .Z(y3));

  AND2_X1 l0 (.A1(g), .A2(h), .ZN(p0));
  DFFP_X1 rq (.D(p0), .CK(clk), .Q(qq));
  INV_X1 l1 (.A(qq), .ZN(p1));
  INV_X1 l2 (.A(p1), .ZN(p2));
  INV_X1 l3 (.A(p2), .ZN(p3));
  INV_X1 l4 (.A(p3), .ZN(p4));
  DFF_X1 r5 (.D(p4), .CK(clk), .Q(q5));
  BUF_X1 l5 (.A(q5), .Z(y4));
endmodule
