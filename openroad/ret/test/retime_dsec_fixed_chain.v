// r1 feeds both the inverter chain and rD, which the test fixes in place.
// The two registers start on one net, so they are rebuilt together or not at
// all: moving r1 across h1 would leave r1 in place and add a register after
// h1.  h1 is pinned instead.  The second cone (r4) still retimes.
// rst_n is a synchronous reset in logic, so every register settles to a
// defined value for the sequential equivalence check.
module top (a, b, rst_n, clk, y, z, w);
  input a;
  input b;
  input rst_n;
  input clk;
  output y;
  output z;
  output w;
  wire d1, q1, qd, n1, n2, n3, n4, n5, d3, q3;
  wire d4, q4, m1, m2, m3, m4, m5, m6, m7, d6, q6;

  AND2_X1 g0 (.A1(a), .A2(rst_n), .ZN(d1));
  DFF_X1 r1 (.D(d1), .CK(clk), .Q(q1));
  DFF_X1 rD (.D(q1), .CK(clk), .Q(qd));
  BUF_X1 g_z (.A(qd), .Z(z));
  INV_X1 h1 (.A(q1), .ZN(n1));
  INV_X1 h2 (.A(n1), .ZN(n2));
  INV_X1 h3 (.A(n2), .ZN(n3));
  INV_X1 h4 (.A(n3), .ZN(n4));
  INV_X1 h5 (.A(n4), .ZN(n5));
  AND2_X1 g8 (.A1(n5), .A2(rst_n), .ZN(d3));
  DFF_X1 r3 (.D(d3), .CK(clk), .Q(q3));
  BUF_X1 g9 (.A(q3), .Z(y));

  AND2_X1 g10 (.A1(b), .A2(rst_n), .ZN(d4));
  DFF_X1 r4 (.D(d4), .CK(clk), .Q(q4));
  INV_X1 k1 (.A(q4), .ZN(m1));
  INV_X1 k2 (.A(m1), .ZN(m2));
  INV_X1 k3 (.A(m2), .ZN(m3));
  INV_X1 k4 (.A(m3), .ZN(m4));
  INV_X1 k5 (.A(m4), .ZN(m5));
  INV_X1 k6 (.A(m5), .ZN(m6));
  INV_X1 k7 (.A(m6), .ZN(m7));
  AND2_X1 g18 (.A1(m7), .A2(rst_n), .ZN(d6));
  DFF_X1 r6 (.D(d6), .CK(clk), .Q(q6));
  BUF_X1 g19 (.A(q6), .Z(w));
endmodule
