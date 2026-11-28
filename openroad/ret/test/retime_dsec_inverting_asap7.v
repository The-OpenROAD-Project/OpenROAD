// Inverting registers on asap7: DFFHQNx1 stores the complement of D (liberty
// next_state "!D") and is the only small flop of the library.
//   cone 1  r1 -> inverters and a buffer: all self-dual, so r1 slides forward
//           and stays a DFFHQNx1.
//   cone 2  r4, r5, r6 -> majority gate: self-dual, so the three registers
//           merge across it into one DFFHQNx1.
// rst_n is a synchronous reset in logic, so every register settles to a
// defined value for the sequential equivalence check.
module top (a, b, c, d, rst_n, clk, y, m);
  input a;
  input b;
  input c;
  input d;
  input rst_n;
  input clk;
  output y;
  output m;
  wire d1, q1n, n1, n2, n3, n4, n5, d3, q3n, q3;
  wire d4, d5, d6, q4n, q5n, q6n, j1, j2, j3, j4, dm, qmn, qm;

  AND2x2_ASAP7_75t_R g0 (.A(a), .B(rst_n), .Y(d1));
  DFFHQNx1_ASAP7_75t_R r1 (.D(d1), .CLK(clk), .QN(q1n));
  INVx1_ASAP7_75t_R g1 (.A(q1n), .Y(n1));
  INVx1_ASAP7_75t_R g2 (.A(n1), .Y(n2));
  BUFx2_ASAP7_75t_R g3 (.A(n2), .Y(n3));
  INVx1_ASAP7_75t_R g4 (.A(n3), .Y(n4));
  INVx1_ASAP7_75t_R g5 (.A(n4), .Y(n5));
  AND2x2_ASAP7_75t_R g6 (.A(n5), .B(rst_n), .Y(d3));
  DFFHQNx1_ASAP7_75t_R r3 (.D(d3), .CLK(clk), .QN(q3n));
  INVx1_ASAP7_75t_R g7 (.A(q3n), .Y(q3));
  BUFx2_ASAP7_75t_R g8 (.A(q3), .Y(y));

  AND2x2_ASAP7_75t_R g10 (.A(b), .B(rst_n), .Y(d4));
  AND2x2_ASAP7_75t_R g11 (.A(c), .B(rst_n), .Y(d5));
  AND2x2_ASAP7_75t_R g12 (.A(d), .B(rst_n), .Y(d6));
  DFFHQNx1_ASAP7_75t_R r4 (.D(d4), .CLK(clk), .QN(q4n));
  DFFHQNx1_ASAP7_75t_R r5 (.D(d5), .CLK(clk), .QN(q5n));
  DFFHQNx1_ASAP7_75t_R r6 (.D(d6), .CLK(clk), .QN(q6n));
  MAJx2_ASAP7_75t_R g13 (.A(q4n), .B(q5n), .C(q6n), .Y(j1));
  INVx1_ASAP7_75t_R g14 (.A(j1), .Y(j2));
  INVx1_ASAP7_75t_R g15 (.A(j2), .Y(j3));
  INVx1_ASAP7_75t_R g16 (.A(j3), .Y(j4));
  AND2x2_ASAP7_75t_R g17 (.A(j4), .B(rst_n), .Y(dm));
  DFFHQNx1_ASAP7_75t_R r7 (.D(dm), .CLK(clk), .QN(qmn));
  INVx1_ASAP7_75t_R g18 (.A(qmn), .Y(qm));
  BUFx2_ASAP7_75t_R g19 (.A(qm), .Y(m));
endmodule
