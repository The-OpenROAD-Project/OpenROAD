// Two registers into an AND2 with four inverters behind it.  The input a
// reaches ra through two inverters, so a slide across g1 lengthens its path.
module top (a, b, rst_n, clk, y);
  input a;
  input b;
  input rst_n;
  input clk;
  output y;
  wire x0, x1, x2, qa, nb, qb, n1, m1, m2, m3, m4, q2;

  AND2_X1 ga (.A1(a), .A2(rst_n), .ZN(x0));
  INV_X1 p1 (.A(x0), .ZN(x1));
  INV_X1 p2 (.A(x1), .ZN(x2));
  DFF_X1 ra (.D(x2), .CK(clk), .Q(qa));
  AND2_X1 gb (.A1(b), .A2(rst_n), .ZN(nb));
  DFF_X1 rb (.D(nb), .CK(clk), .Q(qb));
  AND2_X1 g1 (.A1(qa), .A2(qb), .ZN(n1));
  INV_X1 h1 (.A(n1), .ZN(m1));
  INV_X1 h2 (.A(m1), .ZN(m2));
  INV_X1 h3 (.A(m2), .ZN(m3));
  INV_X1 h4 (.A(m3), .ZN(m4));
  DFF_X1 r2 (.D(m4), .CK(clk), .Q(q2));
  BUF_X1 g5 (.A(q2), .Z(y));
endmodule
