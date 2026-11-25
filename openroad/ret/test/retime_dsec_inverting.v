// Inverting registers on Nangate45: DFF_X1 read through its QN pin presents
// the complement of D one cycle later.
//   cone 1  r1 -> inverters: every gate is self-dual, so r1 could slide as
//           a DFF_X1 that still drives its loads from QN.
//   cone 2  r4, r5 -> NAND: not self-dual, so both would be unmapped to a
//           plain register plus an inverter before they merge.
//   cone 3  r7, r8, r9 -> full adder: carry (majority) and sum (3-input XOR)
//           are both self-dual, so the three registers merge across it and
//           stay inverting.
//   cone 4  r12, r13 -> the full adder of cone 3 with its carry in tied to
//           1'b0: with an input held constant it is not self-dual, so its
//           inverting registers do not keep their class across it.
// rst_n is a synchronous reset in logic, so every register settles to a
// defined value for the sequential equivalence check.
module top (a, b, c, e, f, g, h, i, rst_n, clk, y, z, s, co, t, to);
  input a;
  input b;
  input c;
  input e;
  input f;
  input g;
  input h;
  input i;
  input rst_n;
  input clk;
  output y;
  output z;
  output s;
  output co;
  output t;
  output to;
  wire d1, q1n, n1, n2, n3, n4, n5, d3, q3;
  wire d4, d5, q4n, q5n, m1, m2, m3, m4, m5, d6, q6;
  wire d7, d8, d9, q7n, q8n, q9n, fs, fc, s1, s2, s3, c1, c2, c3, ds, dc, qs, qc;
  wire d12, d13, q12n, q13n, ts, tc, t1, t2, t3, u1, u2, u3, dt, du, qt, qu;

  AND2_X1 g0 (.A1(a), .A2(rst_n), .ZN(d1));
  DFF_X1 r1 (.D(d1), .CK(clk), .QN(q1n));
  INV_X1 g1 (.A(q1n), .ZN(n1));
  INV_X1 g2 (.A(n1), .ZN(n2));
  INV_X1 g3 (.A(n2), .ZN(n3));
  INV_X1 g4 (.A(n3), .ZN(n4));
  INV_X1 g5 (.A(n4), .ZN(n5));
  AND2_X1 g6 (.A1(n5), .A2(rst_n), .ZN(d3));
  DFF_X1 r3 (.D(d3), .CK(clk), .Q(q3));
  BUF_X1 g7 (.A(q3), .Z(y));

  AND2_X1 g10 (.A1(b), .A2(rst_n), .ZN(d4));
  AND2_X1 g11 (.A1(c), .A2(rst_n), .ZN(d5));
  DFF_X1 r4 (.D(d4), .CK(clk), .QN(q4n));
  DFF_X1 r5 (.D(d5), .CK(clk), .QN(q5n));
  NAND2_X1 g12 (.A1(q4n), .A2(q5n), .ZN(m1));
  INV_X1 g13 (.A(m1), .ZN(m2));
  INV_X1 g14 (.A(m2), .ZN(m3));
  INV_X1 g15 (.A(m3), .ZN(m4));
  INV_X1 g16 (.A(m4), .ZN(m5));
  AND2_X1 g17 (.A1(m5), .A2(rst_n), .ZN(d6));
  DFF_X1 r6 (.D(d6), .CK(clk), .Q(q6));
  BUF_X1 g18 (.A(q6), .Z(z));

  AND2_X1 g20 (.A1(e), .A2(rst_n), .ZN(d7));
  AND2_X1 g21 (.A1(f), .A2(rst_n), .ZN(d8));
  AND2_X1 g22 (.A1(g), .A2(rst_n), .ZN(d9));
  DFF_X1 r7 (.D(d7), .CK(clk), .QN(q7n));
  DFF_X1 r8 (.D(d8), .CK(clk), .QN(q8n));
  DFF_X1 r9 (.D(d9), .CK(clk), .QN(q9n));
  FA_X1 g23 (.A(q7n), .B(q8n), .CI(q9n), .S(fs), .CO(fc));
  INV_X1 g24 (.A(fs), .ZN(s1));
  INV_X1 g25 (.A(s1), .ZN(s2));
  INV_X1 g26 (.A(s2), .ZN(s3));
  AND2_X1 g27 (.A1(s3), .A2(rst_n), .ZN(ds));
  DFF_X1 r10 (.D(ds), .CK(clk), .Q(qs));
  BUF_X1 g28 (.A(qs), .Z(s));
  INV_X1 g29 (.A(fc), .ZN(c1));
  INV_X1 g30 (.A(c1), .ZN(c2));
  INV_X1 g31 (.A(c2), .ZN(c3));
  AND2_X1 g32 (.A1(c3), .A2(rst_n), .ZN(dc));
  DFF_X1 r11 (.D(dc), .CK(clk), .Q(qc));
  BUF_X1 g33 (.A(qc), .Z(co));

  AND2_X1 g40 (.A1(h), .A2(rst_n), .ZN(d12));
  AND2_X1 g41 (.A1(i), .A2(rst_n), .ZN(d13));
  DFF_X1 r12 (.D(d12), .CK(clk), .QN(q12n));
  DFF_X1 r13 (.D(d13), .CK(clk), .QN(q13n));
  FA_X1 g43 (.A(q12n), .B(q13n), .CI(1'b0), .S(ts), .CO(tc));
  INV_X1 g44 (.A(ts), .ZN(t1));
  INV_X1 g45 (.A(t1), .ZN(t2));
  INV_X1 g46 (.A(t2), .ZN(t3));
  AND2_X1 g47 (.A1(t3), .A2(rst_n), .ZN(dt));
  DFF_X1 r14 (.D(dt), .CK(clk), .Q(qt));
  BUF_X1 g48 (.A(qt), .Z(t));
  INV_X1 g49 (.A(tc), .ZN(u1));
  INV_X1 g50 (.A(u1), .ZN(u2));
  INV_X1 g51 (.A(u2), .ZN(u3));
  AND2_X1 g52 (.A1(u3), .A2(rst_n), .ZN(du));
  DFF_X1 r15 (.D(du), .CK(clk), .Q(qu));
  BUF_X1 g53 (.A(qu), .Z(to));
endmodule
