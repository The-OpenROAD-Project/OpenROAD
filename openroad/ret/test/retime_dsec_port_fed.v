// rp is fed straight from the input port a, so no gate drives it and it is
// on no edge of the retiming graph.  g2 reads rp and r1: moving r1 across g2
// would leave rp behind and a would reach g2 one cycle late, so g2 is never
// crossed.  The second cone (r4, r5 -> NAND) has a gate in front of each
// register and still retimes.
// rst_n is a synchronous reset in logic, so every register settles to a
// defined value for the sequential equivalence check.
module top (a, b, c, e, rst_n, clk, y, w);
  input a;
  input b;
  input c;
  input e;
  input rst_n;
  input clk;
  output y;
  output w;
  wire qp, d1, q1, n1, n2, n3, n4, n5, n6, d3, q3;
  wire d4, d5, q4, q5, m1, m2, m3, m4, m5, d6, q6;

  DFF_X1 rp (.D(a), .CK(clk), .Q(qp));
  AND2_X1 g0 (.A1(b), .A2(rst_n), .ZN(d1));
  DFF_X1 r1 (.D(d1), .CK(clk), .Q(q1));
  NAND2_X1 g2 (.A1(qp), .A2(q1), .ZN(n1));
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
