// retime_basic plus a transparent latch with a gate in front of it and a
// failing path behind it: the latch is left alone (don't-touch), the gate
// behind it is never crossed, and the register chain is still retimed.
module top (a, b, c, d, clk, y, z);
  input a;
  input b;
  input c;
  input d;
  input clk;
  output y;
  output z;
  wire n0, q1, n1, n2, n3, n4, q2;
  wire m0, l1, m1, m2, m3, m4, q3;

  AND2_X1 g0 (.A1(a), .A2(b), .ZN(n0));
  DFF_X1 r1 (.D(n0), .CK(clk), .Q(q1));
  INV_X1 g1 (.A(q1), .ZN(n1));
  INV_X1 g2 (.A(n1), .ZN(n2));
  INV_X1 g3 (.A(n2), .ZN(n3));
  INV_X1 g4 (.A(n3), .ZN(n4));
  DFF_X1 r2 (.D(n4), .CK(clk), .Q(q2));
  BUF_X1 g5 (.A(q2), .Z(y));

  AND2_X1 h0 (.A1(c), .A2(d), .ZN(m0));
  DLL_X1 lat (.D(m0), .GN(clk), .Q(l1));
  INV_X1 h1 (.A(l1), .ZN(m1));
  INV_X1 h2 (.A(m1), .ZN(m2));
  INV_X1 h3 (.A(m2), .ZN(m3));
  INV_X1 h4 (.A(m3), .ZN(m4));
  DFF_X1 r3 (.D(m4), .CK(clk), .Q(q3));
  BUF_X1 h5 (.A(q3), .Z(z));
endmodule
