// A clock gate feeds part of the design.  The gated chain r1 -> h1..h4 -> r2 is
// retimed and the new register stays on the gated net; g_mix is fed by one
// register on clk and one on gclk, so it is pinned and its chain is left alone.
module top (a, b, en, clk, y, z);
  input a;
  input b;
  input en;
  input clk;
  output y;
  output z;
  wire gclk, n0, q1, m1, m2, m3, m4, q2;
  wire pa, pb, qa, qb, mx, k1n, k2n, k3n, qc;

  CLKGATE_X1 icg (.CK(clk), .E(en), .GCK(gclk));

  AND2_X1 g0 (.A1(a), .A2(b), .ZN(n0));
  DFF_X1 r1 (.D(n0), .CK(gclk), .Q(q1));
  INV_X1 h1 (.A(q1), .ZN(m1));
  INV_X1 h2 (.A(m1), .ZN(m2));
  INV_X1 h3 (.A(m2), .ZN(m3));
  INV_X1 h4 (.A(m3), .ZN(m4));
  DFF_X1 r2 (.D(m4), .CK(gclk), .Q(q2));
  BUF_X1 o1 (.A(q2), .Z(y));

  INV_X1 p0 (.A(a), .ZN(pa));
  DFF_X1 ra (.D(pa), .CK(clk), .Q(qa));
  INV_X1 p1 (.A(b), .ZN(pb));
  DFF_X1 rb (.D(pb), .CK(gclk), .Q(qb));
  AND2_X1 g_mix (.A1(qa), .A2(qb), .ZN(mx));
  INV_X1 k1 (.A(mx), .ZN(k1n));
  INV_X1 k2 (.A(k1n), .ZN(k2n));
  INV_X1 k3 (.A(k2n), .ZN(k3n));
  DFF_X1 rc (.D(k3n), .CK(clk), .Q(qc));
  BUF_X1 o2 (.A(qc), .Z(z));
endmodule
