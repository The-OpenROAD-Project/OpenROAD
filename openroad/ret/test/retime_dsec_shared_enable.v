// r1 is an enable register (sky130hd edfxtp) in front of a failing path.
// Its enable net en has two drivers, g_e and g_x.  rst_n is a synchronous
// reset in logic and forces the enable on while it is asserted, so every
// register settles to a defined value for the sequential equivalence check.
module top (a, en_i, f, rst_n, clk, y);
  input a;
  input en_i;
  input f;
  input rst_n;
  input clk;
  output y;
  wire rst, en, d1, q1, n1, n2, n3, n4, n5, n6, d2, q2;

  sky130_fd_sc_hd__inv_1 g_r (.A(rst_n), .Y(rst));
  sky130_fd_sc_hd__or2_1 g_e (.A(en_i), .B(rst), .X(en));
  sky130_fd_sc_hd__or2_1 g_x (.A(f), .B(rst), .X(en));
  sky130_fd_sc_hd__and2_1 g0 (.A(a), .B(rst_n), .X(d1));
  sky130_fd_sc_hd__edfxtp_1 r1 (.D(d1), .DE(en), .CLK(clk), .Q(q1));
  sky130_fd_sc_hd__inv_1 g1 (.A(q1), .Y(n1));
  sky130_fd_sc_hd__inv_1 g2 (.A(n1), .Y(n2));
  sky130_fd_sc_hd__inv_1 g3 (.A(n2), .Y(n3));
  sky130_fd_sc_hd__inv_1 g4 (.A(n3), .Y(n4));
  sky130_fd_sc_hd__inv_1 g5 (.A(n4), .Y(n5));
  sky130_fd_sc_hd__inv_1 g6 (.A(n5), .Y(n6));
  sky130_fd_sc_hd__and2_1 g7 (.A(n6), .B(rst_n), .X(d2));
  sky130_fd_sc_hd__dfxtp_1 r2 (.D(d2), .CLK(clk), .Q(q2));
  sky130_fd_sc_hd__buf_1 g8 (.A(q2), .X(y));
endmodule
