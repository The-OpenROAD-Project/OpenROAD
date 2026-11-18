// The first register of the netlist, r0, is on clk; the chain
// r1 -> h1..h4 -> r2 is on the gated clock.  Its register moves more than one
// gate deep and must stay on the gated net.
// g1 feeds ra on clk and rb on gclk.  Registers on two clock nets that
// start on one net stay in place, so p1 and p2 are pinned.
module top (a, b, en, clk, y, z, v, w);
  input a;
  input b;
  input en;
  input clk;
  output y;
  output z;
  output v;
  output w;
  wire gclk, q0, n0, q1, m1, m2, m3, m4, q2;
  wire n1, qa, qb, pa, pb, qc, qd;

  DFF_X1 r0 (.D(b), .CK(clk), .Q(q0));
  BUF_X1 o0 (.A(q0), .Z(z));

  CLKGATE_X1 icg (.CK(clk), .E(en), .GCK(gclk));

  AND2_X1 g0 (.A1(a), .A2(b), .ZN(n0));
  DFF_X1 r1 (.D(n0), .CK(gclk), .Q(q1));
  INV_X1 h1 (.A(q1), .ZN(m1));
  INV_X1 h2 (.A(m1), .ZN(m2));
  INV_X1 h3 (.A(m2), .ZN(m3));
  INV_X1 h4 (.A(m3), .ZN(m4));
  DFF_X1 r2 (.D(m4), .CK(gclk), .Q(q2));
  BUF_X1 o1 (.A(q2), .Z(y));

  OR2_X1 g1 (.A1(a), .A2(b), .ZN(n1));
  DFF_X1 ra (.D(n1), .CK(clk), .Q(qa));
  DFF_X1 rb (.D(n1), .CK(gclk), .Q(qb));
  INV_X1 p1 (.A(qa), .ZN(pa));
  INV_X1 p2 (.A(qb), .ZN(pb));
  DFF_X1 rc (.D(pa), .CK(clk), .Q(qc));
  DFF_X1 rd (.D(pb), .CK(gclk), .Q(qd));
  BUF_X1 o2 (.A(qc), .Z(v));
  BUF_X1 o3 (.A(qd), .Z(w));
endmodule
