// t reads input c, so it is pinned and cannot be crossed.  r1 can slide
// across i1, then j1 and j2 toward r3; after the first slide a register
// drives t directly.  rst_n is a synchronous reset in logic, so every
// register settles to a defined value for the sequential equivalence check.
module top (a, c, rst_n, clk, y, z);
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
