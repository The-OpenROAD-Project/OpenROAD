// A clock gate feeds part of the design.  The gated chain r1 -> h1..h4 -> r2 is
// retimed and the new register stays on the gated net; g_mix is fed by one
// register on clk and one on gclk, so it is pinned and its chain is left alone.
module top (a, b, en, clk, y, z, w, v, u, s, t);
  input a;
  input b;
  input en;
  input clk;
  output y;
  output z;
  output w;
  output v;
  output u;
  output s;
  output t;
  wire gclk, n0, q1, m1, m2, m3, m4, q2;
  wire pa, pb, qa, qb, mx, k1n, k2n, k3n, qc;
  wire sa, sb, qs1, qs2, s1n, mx2, t1n, t2n, t3n, t4n, qs3;
  wire nf, qf1, qf2, f1n, f2n, f3n, f4n, qf3, qf4;
  wire a_i, qi1, i1n, i2n, i3n, i4n, qi2;
  wire a_j, qj1, j1n, j2n, j3n, j4n, j5n, qj2;

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

  // rs1 (clk) can cross s1, and would then meet rs2 (gclk) at g_mix2: that
  // gate is pinned too, although only one register feeds it directly.
  INV_X1 s0 (.A(a), .ZN(sa));
  DFF_X1 rs1 (.D(sa), .CK(clk), .Q(qs1));
  INV_X1 s1 (.A(qs1), .ZN(s1n));
  INV_X1 s2 (.A(b), .ZN(sb));
  DFF_X1 rs2 (.D(sb), .CK(gclk), .Q(qs2));
  AND2_X1 g_mix2 (.A1(s1n), .A2(qs2), .ZN(mx2));
  INV_X1 t1 (.A(mx2), .ZN(t1n));
  INV_X1 t2 (.A(t1n), .ZN(t2n));
  INV_X1 t3 (.A(t2n), .ZN(t3n));
  INV_X1 t4 (.A(t3n), .ZN(t4n));
  DFF_X1 rs3 (.D(t4n), .CK(clk), .Q(qs3));
  BUF_X1 o3 (.A(qs3), .Z(w));

  // g_fan feeds rf1 on clk and rf2 on gclk.  Registers on two clock nets
  // that start on one net stay in place, so f1 and f2 are pinned.
  OR2_X1 g_fan (.A1(a), .A2(b), .ZN(nf));
  DFF_X1 rf1 (.D(nf), .CK(clk), .Q(qf1));
  DFF_X1 rf2 (.D(nf), .CK(gclk), .Q(qf2));
  INV_X1 f1 (.A(qf1), .ZN(f1n));
  INV_X1 f2 (.A(qf2), .ZN(f2n));
  INV_X1 f3 (.A(f1n), .ZN(f3n));
  INV_X1 f4 (.A(f2n), .ZN(f4n));
  DFF_X1 rf3 (.D(f3n), .CK(clk), .Q(qf3));
  DFF_X1 rf4 (.D(f4n), .CK(gclk), .Q(qf4));
  BUF_X1 o4 (.A(qf3), .Z(v));
  BUF_X1 o5 (.A(qf4), .Z(u));

  // The groups are rebuilt in the order of their nets (a_i, a_j, i3n, j4n):
  // ri1 and rj1 are removed before the registers that replace them are
  // made, the one on clk first.  The new register of the gated chain must
  // still be on gclk.
  BUF_X1 bi (.A(a), .Z(a_i));
  DFF_X1 ri1 (.D(a_i), .CK(clk), .Q(qi1));
  INV_X1 i1 (.A(qi1), .ZN(i1n));
  INV_X1 i2 (.A(i1n), .ZN(i2n));
  INV_X1 i3 (.A(i2n), .ZN(i3n));
  INV_X1 i4 (.A(i3n), .ZN(i4n));
  DFF_X1 ri2 (.D(i4n), .CK(clk), .Q(qi2));
  BUF_X1 o6 (.A(qi2), .Z(s));

  BUF_X1 bj (.A(b), .Z(a_j));
  DFF_X1 rj1 (.D(a_j), .CK(gclk), .Q(qj1));
  INV_X1 j1 (.A(qj1), .ZN(j1n));
  INV_X1 j2 (.A(j1n), .ZN(j2n));
  INV_X1 j3 (.A(j2n), .ZN(j3n));
  INV_X1 j4 (.A(j3n), .ZN(j4n));
  INV_X1 j5 (.A(j4n), .ZN(j5n));
  DFF_X1 rj2 (.D(j5n), .CK(gclk), .Q(qj2));
  BUF_X1 o7 (.A(qj2), .Z(t));
endmodule
