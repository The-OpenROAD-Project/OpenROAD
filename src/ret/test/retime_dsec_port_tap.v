// A primary output tapped from the middle of a register chain.  z reads rA
// one cycle after a, so the chain rA -> rB -> rD in front of g2 must stay
// as it is: moving rD across g2 would rebuild the chain two deep and z
// would read it at the wrong depth.
// The tapped cone is the longer one, so it is the path the search wants to
// fix; the second cone (r4, r5 -> NAND) has no tap and still retimes.
// rst_n is a synchronous reset in logic, so every register settles to a
// defined value for the sequential equivalence check.
module top (a, b, c, e, rst_n, clk, y, z, w);
  input a;
  input b;
  input c;
  input e;
  input rst_n;
  input clk;
  output y;
  output z;
  output w;
  wire d1, d2, qa, qb, qd, qc, n1, n2, n3, n4, n5, n6, d3, q3;
  wire d4, d5, q4, q5, m1, m2, m3, m4, m5, d6, q6;

  AND2_X1 g0 (.A1(a), .A2(rst_n), .ZN(d1));
  AND2_X1 g1 (.A1(b), .A2(rst_n), .ZN(d2));
  DFF_X1 rA (.D(d1), .CK(clk), .Q(qa));
  DFF_X1 rB (.D(qa), .CK(clk), .Q(qb));
  DFF_X1 rD (.D(qb), .CK(clk), .Q(qd));
  DFF_X1 rC (.D(d2), .CK(clk), .Q(qc));
  assign z = qa;
  NAND2_X1 g2 (.A1(qd), .A2(qc), .ZN(n1));
  INV_X1 g3 (.A(n1), .ZN(n2));
  INV_X1 g4 (.A(n2), .ZN(n3));
  INV_X1 g5 (.A(n3), .ZN(n4));
  INV_X1 g6 (.A(n4), .ZN(n5));
  INV_X1 g6b (.A(n5), .ZN(n6));
  AND2_X1 g7 (.A1(n6), .A2(rst_n), .ZN(d3));
  DFF_X1 r3 (.D(d3), .CK(clk), .Q(q3));
  BUF_X1 g8 (.A(q3), .Z(y));

  AND2_X1 g10 (.A1(c), .A2(rst_n), .ZN(d4));
  AND2_X1 g11 (.A1(e), .A2(rst_n), .ZN(d5));
  DFF_X1 r4 (.D(d4), .CK(clk), .Q(q4));
  DFF_X1 r5 (.D(d5), .CK(clk), .Q(q5));
  NAND2_X1 g12 (.A1(q4), .A2(q5), .ZN(m1));
  INV_X1 g13 (.A(m1), .ZN(m2));
  INV_X1 g14 (.A(m2), .ZN(m3));
  INV_X1 g15 (.A(m3), .ZN(m4));
  INV_X1 g16 (.A(m4), .ZN(m5));
  AND2_X1 g17 (.A1(m5), .A2(rst_n), .ZN(d6));
  DFF_X1 r6 (.D(d6), .CK(clk), .Q(q6));
  BUF_X1 g18 (.A(q6), .Z(w));
endmodule
