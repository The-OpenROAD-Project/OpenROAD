// Inverting registers on Nangate45: DFF_X1 read through its QN pin presents
// the complement of D one cycle later.
//   cone 1  r1, r2, r3 -> full adder: carry (majority) and sum (3-input XOR)
//           are both self-dual, so the three registers merge across it and
//           stay inverting.
//   cone 2  r6, r7 -> the full adder of cone 1 with its carry in tied to
//           1'b0: with an input held constant it is not self-dual, so its
//           inverting registers do not keep their class across it.
// rst_n is a synchronous reset in logic, so every register settles to a
// defined value for the sequential equivalence check.
module top (a, b, c, d, e, rst_n, clk, s, co, t, to);
  input a;
  input b;
  input c;
  input d;
  input e;
  input rst_n;
  input clk;
  output s;
  output co;
  output t;
  output to;
  wire d1, d2, d3, q1n, q2n, q3n, fs, fc;
  wire s1, s2, s3, c1, c2, c3, ds, dc, qs, qc;
  wire d6, d7, q6n, q7n, ts, tc;
  wire t1, t2, t3, u1, u2, u3, dt, du, qt, qu;

  AND2_X1 g0 (.A1(a), .A2(rst_n), .ZN(d1));
  AND2_X1 g1 (.A1(b), .A2(rst_n), .ZN(d2));
  AND2_X1 g2 (.A1(c), .A2(rst_n), .ZN(d3));
  DFF_X1 r1 (.D(d1), .CK(clk), .QN(q1n));
  DFF_X1 r2 (.D(d2), .CK(clk), .QN(q2n));
  DFF_X1 r3 (.D(d3), .CK(clk), .QN(q3n));
  FA_X1 g3 (.A(q1n), .B(q2n), .CI(q3n), .S(fs), .CO(fc));
  INV_X1 g4 (.A(fs), .ZN(s1));
  INV_X1 g5 (.A(s1), .ZN(s2));
  INV_X1 g6 (.A(s2), .ZN(s3));
  AND2_X1 g7 (.A1(s3), .A2(rst_n), .ZN(ds));
  DFF_X1 r4 (.D(ds), .CK(clk), .Q(qs));
  BUF_X1 g8 (.A(qs), .Z(s));
  INV_X1 g9 (.A(fc), .ZN(c1));
  INV_X1 g10 (.A(c1), .ZN(c2));
  INV_X1 g11 (.A(c2), .ZN(c3));
  AND2_X1 g12 (.A1(c3), .A2(rst_n), .ZN(dc));
  DFF_X1 r5 (.D(dc), .CK(clk), .Q(qc));
  BUF_X1 g13 (.A(qc), .Z(co));

  AND2_X1 g20 (.A1(d), .A2(rst_n), .ZN(d6));
  AND2_X1 g21 (.A1(e), .A2(rst_n), .ZN(d7));
  DFF_X1 r6 (.D(d6), .CK(clk), .QN(q6n));
  DFF_X1 r7 (.D(d7), .CK(clk), .QN(q7n));
  FA_X1 g22 (.A(q6n), .B(q7n), .CI(1'b0), .S(ts), .CO(tc));
  INV_X1 g23 (.A(ts), .ZN(t1));
  INV_X1 g24 (.A(t1), .ZN(t2));
  INV_X1 g25 (.A(t2), .ZN(t3));
  AND2_X1 g26 (.A1(t3), .A2(rst_n), .ZN(dt));
  DFF_X1 r8 (.D(dt), .CK(clk), .Q(qt));
  BUF_X1 g27 (.A(qt), .Z(t));
  INV_X1 g28 (.A(tc), .ZN(u1));
  INV_X1 g29 (.A(u1), .ZN(u2));
  INV_X1 g30 (.A(u2), .ZN(u3));
  AND2_X1 g31 (.A1(u3), .A2(rst_n), .ZN(du));
  DFF_X1 r9 (.D(du), .CK(clk), .Q(qu));
  BUF_X1 g32 (.A(qu), .Z(to));
endmodule
