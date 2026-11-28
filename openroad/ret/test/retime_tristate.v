// Three failing paths behind cells retime leaves alone.  t1 is a tri-state
// driver with a register on each input.  rt is a register whose Q drives
// only while OE is high.  u1 is a delay cell whose output has no liberty
// function.  DFFT_X1 and DLY_X1 are in retime_tristate.lib.
module top (a, b, e, c, d, oe, f, h, clk, y, z, w);
  input a;
  input b;
  input e;
  input c;
  input d;
  input oe;
  input f;
  input h;
  input clk;
  output y;
  output z;
  output w;
  wire d1, q1, de, qe, n0, n1, n2, n3, n4, q2;
  wire dt, qt, m1, m2, m3, m4, q3;
  wire du, qu, p0, p1, p2, p3, p4, q4;

  AND2_X1 g0 (.A1(a), .A2(b), .ZN(d1));
  DFF_X1 r1 (.D(d1), .CK(clk), .Q(q1));
  BUF_X1 g1 (.A(e), .Z(de));
  DFF_X1 re (.D(de), .CK(clk), .Q(qe));
  TBUF_X1 t1 (.A(q1), .EN(qe), .Z(n0));
  INV_X1 i1 (.A(n0), .ZN(n1));
  INV_X1 i2 (.A(n1), .ZN(n2));
  INV_X1 i3 (.A(n2), .ZN(n3));
  INV_X1 i4 (.A(n3), .ZN(n4));
  DFF_X1 r2 (.D(n4), .CK(clk), .Q(q2));
  BUF_X1 g5 (.A(q2), .Z(y));

  AND2_X1 h0 (.A1(c), .A2(d), .ZN(dt));
  DFFT_X1 rt (.D(dt), .CK(clk), .OE(oe), .Q(qt));
  INV_X1 j1 (.A(qt), .ZN(m1));
  INV_X1 j2 (.A(m1), .ZN(m2));
  INV_X1 j3 (.A(m2), .ZN(m3));
  INV_X1 j4 (.A(m3), .ZN(m4));
  DFF_X1 r3 (.D(m4), .CK(clk), .Q(q3));
  BUF_X1 h5 (.A(q3), .Z(z));

  AND2_X1 k0 (.A1(f), .A2(h), .ZN(du));
  DFF_X1 ru (.D(du), .CK(clk), .Q(qu));
  DLY_X1 u1 (.A(qu), .Z(p0));
  INV_X1 k1 (.A(p0), .ZN(p1));
  INV_X1 k2 (.A(p1), .ZN(p2));
  INV_X1 k3 (.A(p2), .ZN(p3));
  INV_X1 k4 (.A(p3), .ZN(p4));
  DFF_X1 r4 (.D(p4), .CK(clk), .Q(q4));
  BUF_X1 k5 (.A(q4), .Z(w));
endmodule
