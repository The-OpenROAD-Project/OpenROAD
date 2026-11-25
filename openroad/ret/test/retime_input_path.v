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

// t also reads input c, so it is pinned and cannot be crossed.  r1 can slide
// across i1, then j1 and j2 toward r3; after the first slide t is fed only
// by a register.  rst_n is a synchronous reset in logic, so every register
// settles to a defined value for the sequential equivalence check.
module top_pinned (a, c, rst_n, clk, y, z);
  input a;
  input c;
  input rst_n;
  input clk;
  output y;
  output z;
  wire d1, q1, n1, t0, m1, m2, d2, q2;
  wire k1, k2, d3, q3;

  AND2_X1 g0 (.A1(a), .A2(rst_n), .ZN(d1));
  DFF_X1 r1 (.D(d1), .CK(clk), .Q(q1));
  INV_X1 i1 (.A(q1), .ZN(n1));
  AND2_X1 t (.A1(n1), .A2(c), .ZN(t0));
  INV_X1 h1 (.A(t0), .ZN(m1));
  INV_X1 h2 (.A(m1), .ZN(m2));
  AND2_X1 g5 (.A1(m2), .A2(rst_n), .ZN(d2));
  DFF_X1 r2 (.D(d2), .CK(clk), .Q(q2));
  BUF_X1 o1 (.A(q2), .Z(y));

  INV_X1 j1 (.A(n1), .ZN(k1));
  INV_X1 j2 (.A(k1), .ZN(k2));
  AND2_X1 g6 (.A1(k2), .A2(rst_n), .ZN(d3));
  DFF_X1 r3 (.D(d3), .CK(clk), .Q(q3));
  BUF_X1 o2 (.A(q3), .Z(z));
endmodule
