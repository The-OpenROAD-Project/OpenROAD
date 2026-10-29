// The longest combinational path starts at input c (six inverters into r3).
// A forward move cannot shorten it, so it must not stop retime from balancing
// the register-to-register chain r1 -> h1..h4 -> r2 (one slide, as in retime_basic).
module top (a, b, c, clk, y, z);
  input a;
  input b;
  input c;
  input clk;
  output y;
  output z;
  wire n0, q1, m1, m2, m3, m4, q2;
  wire c1, c2, c3, c4, c5, c6, q3;

  AND2_X1 g0 (.A1(a), .A2(b), .ZN(n0));
  DFF_X1 r1 (.D(n0), .CK(clk), .Q(q1));
  INV_X1 h1 (.A(q1), .ZN(m1));
  INV_X1 h2 (.A(m1), .ZN(m2));
  INV_X1 h3 (.A(m2), .ZN(m3));
  INV_X1 h4 (.A(m3), .ZN(m4));
  DFF_X1 r2 (.D(m4), .CK(clk), .Q(q2));
  BUF_X1 o1 (.A(q2), .Z(y));

  INV_X1 p1 (.A(c), .ZN(c1));
  INV_X1 p2 (.A(c1), .ZN(c2));
  INV_X1 p3 (.A(c2), .ZN(c3));
  INV_X1 p4 (.A(c3), .ZN(c4));
  INV_X1 p5 (.A(c4), .ZN(c5));
  INV_X1 p6 (.A(c5), .ZN(c6));
  DFF_X1 r3 (.D(c6), .CK(clk), .Q(q3));
  BUF_X1 o2 (.A(q3), .Z(z));
endmodule
