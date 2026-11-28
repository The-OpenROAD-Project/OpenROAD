// Two copies of one failing cone (Nangate45 plus DFFG_X1, clocked by
// "CK & EN").
//   cone 1  r1 (DFF_X1) -> inverters -> r2: r1 slides forward.
//   cone 2  rg (DFFG_X1) -> inverters -> r3: rg never moves, so the
//           inverters behind it are never crossed.
module top (a, b, en_i, rst_n, clk, y, z);
  input a;
  input b;
  input en_i;
  input rst_n;
  input clk;
  output y;
  output z;
  wire rst, en, d1, q1, n1, n2, n3, n4, d2, q2;
  wire d3, qg, m1, m2, m3, m4, d4, q4;

  INV_X1 g_r (.A(rst_n), .ZN(rst));
  OR2_X1 g_e (.A1(en_i), .A2(rst), .ZN(en));

  AND2_X1 g0 (.A1(a), .A2(rst_n), .ZN(d1));
  DFF_X1 r1 (.D(d1), .CK(clk), .Q(q1));
  INV_X1 g1 (.A(q1), .ZN(n1));
  INV_X1 g2 (.A(n1), .ZN(n2));
  INV_X1 g3 (.A(n2), .ZN(n3));
  INV_X1 g4 (.A(n3), .ZN(n4));
  AND2_X1 g5 (.A1(n4), .A2(rst_n), .ZN(d2));
  DFF_X1 r2 (.D(d2), .CK(clk), .Q(q2));
  BUF_X1 g6 (.A(q2), .Z(y));

  AND2_X1 g7 (.A1(b), .A2(rst_n), .ZN(d3));
  DFFG_X1 rg (.D(d3), .CK(clk), .EN(en), .Q(qg));
  INV_X1 g8 (.A(qg), .ZN(m1));
  INV_X1 g9 (.A(m1), .ZN(m2));
  INV_X1 g10 (.A(m2), .ZN(m3));
  INV_X1 g11 (.A(m3), .ZN(m4));
  AND2_X1 g12 (.A1(m4), .A2(rst_n), .ZN(d4));
  DFF_X1 r3 (.D(d4), .CK(clk), .Q(q4));
  BUF_X1 g13 (.A(q4), .Z(z));
endmodule
