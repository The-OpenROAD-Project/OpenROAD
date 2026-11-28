// Registers with an asynchronous reset (Nangate45 DFFR_X1).  A reset tied
// high never fires, so the register moves like one without a reset.
//   cone 1  r1, r2 have RN tied high: they merge across the NAND as the base
//           DFF_X1, and the tie cells that only they read are removed.
//   cone 2  r4 has RN tied high and is read through QN: a cell with a reset
//           does not keep its class, so r4 is unmapped to a DFF_X1 plus an
//           inverter before it slides across the buffer and the inverters.
//   cone 3  r6 is reset by rst_n: it stays.
// rst_n is also a synchronous reset in logic, so every register settles to a
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
  wire hi1, hi2, hi4;
  wire d1, d2, q1, q2, n1, n2, n3, n4, n5, n6, d3, q3;
  wire d4, q4n, m1, m2, m3, m4, m5, d5, q5;
  wire d6, q6, p1, p2, p3, p4, p5, d7, q7;

  LOGIC1_X1 t1 (.Z(hi1));
  LOGIC1_X1 t2 (.Z(hi2));
  LOGIC1_X1 t4 (.Z(hi4));

  AND2_X1 g0 (.A1(a), .A2(rst_n), .ZN(d1));
  AND2_X1 g1 (.A1(b), .A2(rst_n), .ZN(d2));
  DFFR_X1 r1 (.D(d1), .RN(hi1), .CK(clk), .Q(q1));
  DFFR_X1 r2 (.D(d2), .RN(hi2), .CK(clk), .Q(q2));
  NAND2_X1 g2 (.A1(q1), .A2(q2), .ZN(n1));
  INV_X1 g3 (.A(n1), .ZN(n2));
  INV_X1 g4 (.A(n2), .ZN(n3));
  INV_X1 g5 (.A(n3), .ZN(n4));
  INV_X1 g6 (.A(n4), .ZN(n5));
  INV_X1 g7 (.A(n5), .ZN(n6));
  AND2_X1 g8 (.A1(n6), .A2(rst_n), .ZN(d3));
  DFF_X1 r3 (.D(d3), .CK(clk), .Q(q3));
  BUF_X1 g9 (.A(q3), .Z(y));

  AND2_X1 g10 (.A1(c), .A2(rst_n), .ZN(d4));
  DFFR_X1 r4 (.D(d4), .RN(hi4), .CK(clk), .QN(q4n));
  BUF_X1 g11 (.A(q4n), .Z(m1));
  INV_X1 g12 (.A(m1), .ZN(m2));
  INV_X1 g13 (.A(m2), .ZN(m3));
  INV_X1 g14 (.A(m3), .ZN(m4));
  INV_X1 g15 (.A(m4), .ZN(m5));
  AND2_X1 g16 (.A1(m5), .A2(rst_n), .ZN(d5));
  DFF_X1 r5 (.D(d5), .CK(clk), .Q(q5));
  BUF_X1 g17 (.A(q5), .Z(z));

  AND2_X1 g20 (.A1(e), .A2(rst_n), .ZN(d6));
  DFFR_X1 r6 (.D(d6), .RN(rst_n), .CK(clk), .Q(q6));
  INV_X1 g21 (.A(q6), .ZN(p1));
  INV_X1 g22 (.A(p1), .ZN(p2));
  INV_X1 g23 (.A(p2), .ZN(p3));
  INV_X1 g24 (.A(p3), .ZN(p4));
  INV_X1 g25 (.A(p4), .ZN(p5));
  AND2_X1 g26 (.A1(p5), .A2(rst_n), .ZN(d7));
  DFF_X1 r7 (.D(d7), .CK(clk), .Q(q7));
  BUF_X1 g27 (.A(q7), .Z(w));
endmodule
