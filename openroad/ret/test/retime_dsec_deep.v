// Two registers cross the same gate.  Each input of the NAND g2 has a chain
// of a plain register (rA1, rB1) followed by an enable register (rA2, rB2,
// both enabled by `en`).  The cone behind g2 is long enough that the search
// first takes rA2 and rB2 across g2 and then rA1 and rB1 as well.  The first
// pair merges into one enable register on `en`, the second pair into one
// plain register, and they keep their order.  rst_n is a synchronous reset in
// logic and the enable is forced on while it is asserted, so every register
// settles to a defined value for the sequential equivalence check.
module top (a, b, en_i, rst_n, clk, y);
  input a;
  input b;
  input en_i;
  input rst_n;
  input clk;
  output y;
  wire rst, en, d1, d2, qa1, qa2, qb1, qb2;
  wire n1, n2, n3, n4, n5, n6, n7, n8, d3, q3;

  sky130_fd_sc_hd__inv_1 g_r (.A(rst_n), .Y(rst));
  sky130_fd_sc_hd__or2_1 g_e (.A(en_i), .B(rst), .X(en));
  sky130_fd_sc_hd__and2_1 g0 (.A(a), .B(rst_n), .X(d1));
  sky130_fd_sc_hd__and2_1 g1 (.A(b), .B(rst_n), .X(d2));
  sky130_fd_sc_hd__dfxtp_1 rA1 (.D(d1), .CLK(clk), .Q(qa1));
  sky130_fd_sc_hd__edfxtp_1 rA2 (.D(qa1), .DE(en), .CLK(clk), .Q(qa2));
  sky130_fd_sc_hd__dfxtp_1 rB1 (.D(d2), .CLK(clk), .Q(qb1));
  sky130_fd_sc_hd__edfxtp_1 rB2 (.D(qb1), .DE(en), .CLK(clk), .Q(qb2));

  sky130_fd_sc_hd__nand2_1 g2 (.A(qa2), .B(qb2), .Y(n1));
  sky130_fd_sc_hd__inv_1 g3 (.A(n1), .Y(n2));
  sky130_fd_sc_hd__inv_1 g4 (.A(n2), .Y(n3));
  sky130_fd_sc_hd__inv_1 g5 (.A(n3), .Y(n4));
  sky130_fd_sc_hd__inv_1 g6 (.A(n4), .Y(n5));
  sky130_fd_sc_hd__inv_1 g7 (.A(n5), .Y(n6));
  sky130_fd_sc_hd__inv_1 g8 (.A(n6), .Y(n7));
  sky130_fd_sc_hd__inv_1 g9 (.A(n7), .Y(n8));
  sky130_fd_sc_hd__and2_1 g10 (.A(n8), .B(rst_n), .X(d3));
  sky130_fd_sc_hd__dfxtp_1 r3 (.D(d3), .CLK(clk), .Q(q3));
  sky130_fd_sc_hd__buf_1 g11 (.A(q3), .X(y));
endmodule
