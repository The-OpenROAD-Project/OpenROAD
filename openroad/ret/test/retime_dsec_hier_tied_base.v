// ihp-sg13g2, hierarchical.  The library has no flip-flop without an
// asynchronous reset, so every register in `core` is a sg13g2_dfrbp_1 with
// RESET_B on a tie cell of its own.  r1 is read through Q_N: it is unmapped
// to the base cell, a new tie cell and an inverter, all created in `core`,
// and then merges with r2 across the NAND.  The tie cells of the registers
// that go are removed with them.  rst_n is a synchronous reset in logic, so
// every register settles to a defined value for the sequential equivalence
// check.
module core (a, b, rn, ck, y);
  input a;
  input b;
  input rn;
  input ck;
  output y;
  wire hi1, hi2, hi3;
  wire d1, d2, q1n, q2, n1, n2, n3, n4, n5, n6, d3, q3;

  sg13g2_tiehi t1 (.L_HI(hi1));
  sg13g2_tiehi t2 (.L_HI(hi2));
  sg13g2_tiehi t3 (.L_HI(hi3));

  sg13g2_and2_1 g0 (.A(a), .B(rn), .X(d1));
  sg13g2_and2_1 g1 (.A(b), .B(rn), .X(d2));
  sg13g2_dfrbp_1 r1 (.D(d1), .RESET_B(hi1), .CLK(ck), .Q_N(q1n));
  sg13g2_dfrbp_1 r2 (.D(d2), .RESET_B(hi2), .CLK(ck), .Q(q2));
  sg13g2_nand2_1 g2 (.A(q1n), .B(q2), .Y(n1));
  sg13g2_inv_1 g3 (.A(n1), .Y(n2));
  sg13g2_inv_1 g4 (.A(n2), .Y(n3));
  sg13g2_inv_1 g5 (.A(n3), .Y(n4));
  sg13g2_inv_1 g6 (.A(n4), .Y(n5));
  sg13g2_inv_1 g7 (.A(n5), .Y(n6));
  sg13g2_and2_1 g8 (.A(n6), .B(rn), .X(d3));
  sg13g2_dfrbp_1 r3 (.D(d3), .RESET_B(hi3), .CLK(ck), .Q(q3));
  sg13g2_buf_1 g9 (.A(q3), .X(y));
endmodule

module top (a, b, rst_n, clk, y);
  input a;
  input b;
  input rst_n;
  input clk;
  output y;

  core u_core (.a(a), .b(b), .rn(rst_n), .ck(clk), .y(y));
endmodule
