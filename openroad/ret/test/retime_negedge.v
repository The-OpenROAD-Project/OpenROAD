// A negative-edge register stays in place while positive-edge ones retime.
//   cone 1  r1, r2 (DFFHQx4, rising edge) -> NAND and inverters: they merge
//           forward across the NAND.
//   cone 2  rn (DFFLQx4, falling edge) -> inverters: rn never moves, and the
//           gates it feeds are never crossed.
// rst_n is a synchronous reset in logic, so every register settles to a
// defined value for the sequential equivalence check.
module top (a, b, c, rst_n, clk, y, z);
  input a;
  input b;
  input c;
  input rst_n;
  input clk;
  output y;
  output z;
  wire d1, d2, q1, q2, n1, n2, n3, n4, n5, n6, d3, q3;
  wire dn, qn, m1, m2, m3, m4, d4, q4;

  AND2x2_ASAP7_75t_R g0 (.A(a), .B(rst_n), .Y(d1));
  AND2x2_ASAP7_75t_R g1 (.A(b), .B(rst_n), .Y(d2));
  DFFHQx4_ASAP7_75t_R r1 (.D(d1), .CLK(clk), .Q(q1));
  DFFHQx4_ASAP7_75t_R r2 (.D(d2), .CLK(clk), .Q(q2));
  NAND2xp5_ASAP7_75t_R g2 (.A(q1), .B(q2), .Y(n1));
  INVx1_ASAP7_75t_R g3 (.A(n1), .Y(n2));
  INVx1_ASAP7_75t_R g4 (.A(n2), .Y(n3));
  INVx1_ASAP7_75t_R g5 (.A(n3), .Y(n4));
  INVx1_ASAP7_75t_R g6 (.A(n4), .Y(n5));
  INVx1_ASAP7_75t_R g7 (.A(n5), .Y(n6));
  AND2x2_ASAP7_75t_R g8 (.A(n6), .B(rst_n), .Y(d3));
  DFFHQx4_ASAP7_75t_R r3 (.D(d3), .CLK(clk), .Q(q3));
  BUFx2_ASAP7_75t_R g9 (.A(q3), .Y(y));

  AND2x2_ASAP7_75t_R g10 (.A(c), .B(rst_n), .Y(dn));
  DFFLQx4_ASAP7_75t_R rn (.D(dn), .CLK(clk), .Q(qn));
  INVx1_ASAP7_75t_R g11 (.A(qn), .Y(m1));
  INVx1_ASAP7_75t_R g12 (.A(m1), .Y(m2));
  INVx1_ASAP7_75t_R g13 (.A(m2), .Y(m3));
  INVx1_ASAP7_75t_R g14 (.A(m3), .Y(m4));
  AND2x2_ASAP7_75t_R g15 (.A(m4), .B(rst_n), .Y(d4));
  DFFHQx4_ASAP7_75t_R r4 (.D(d4), .CLK(clk), .Q(q4));
  BUFx2_ASAP7_75t_R g16 (.A(q4), .Y(z));
endmodule
