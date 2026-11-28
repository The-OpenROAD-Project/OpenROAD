// Two plain registers feed one NAND, followed by a longer cone.  Forward
// retiming merges r1 and r2 into one register after the NAND and slides it
// on.  rst_n is a synchronous reset in logic, so every register settles to a
// defined value and the result can be checked with sequential equivalence.
module top (a, b, rst_n, clk, y);
  input a;
  input b;
  input rst_n;
  input clk;
  output y;
  wire d1, d2, q1, q2, n1, n2, n3, n4, n5, n6, d3, q3;

  AND2_X1 g0 (.A1(a), .A2(rst_n), .ZN(d1));
  AND2_X1 g1 (.A1(b), .A2(rst_n), .ZN(d2));
  DFF_X1 r1 (.D(d1), .CK(clk), .Q(q1));
  DFF_X1 r2 (.D(d2), .CK(clk), .Q(q2));
  NAND2_X1 g2 (.A1(q1), .A2(q2), .ZN(n1));
  INV_X1 g3 (.A(n1), .ZN(n2));
  INV_X1 g4 (.A(n2), .ZN(n3));
  INV_X1 g5 (.A(n3), .ZN(n4));
  INV_X1 g6 (.A(n4), .ZN(n5));
  INV_X1 g7 (.A(n5), .ZN(n6));
  AND2_X1 g8 (.A1(n6), .A2(rst_n), .ZN(d3));
  DFF_X1 r3 (.D(d3), .CK(clk), .Q(q3));
  BUF_X1 g9 (.A(q3), .Z(y));
endmodule
