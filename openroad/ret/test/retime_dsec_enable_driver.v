// A register whose output drives the enable pin of another register.  rE's
// Q feeds both the NAND g2 and, directly, the DE pin of rX; moving rE across
// g2 would hand the enable the value one cycle early, so rE (and rF, which
// merges with it) stay.  The second cone (r4, r5 -> NOR) still retimes.
// rst_n is a synchronous reset in logic; while it is asserted rE and rX's D
// are 0, so every register settles to a defined value for the sequential
// equivalence check.
module top (a, b, c, d, e, rst_n, clk, y, z, w);
  input a;
  input b;
  input c;
  input d;
  input e;
  input rst_n;
  input clk;
  output y;
  output z;
  output w;
  wire d1, d2, dx, qe, qf, qx, n1, n2, n3, n4, n5, n6, d3, q3;
  wire d4, d5, q4, q5, m1, m2, m3, m4, m5, m6, d6, q6;

  sky130_fd_sc_hd__and2_1 g0 (.A(a), .B(rst_n), .X(d1));
  sky130_fd_sc_hd__and2_1 g1 (.A(b), .B(rst_n), .X(d2));
  sky130_fd_sc_hd__and2_1 g1x (.A(c), .B(rst_n), .X(dx));
  sky130_fd_sc_hd__dfxtp_1 rE (.D(d1), .CLK(clk), .Q(qe));
  sky130_fd_sc_hd__dfxtp_1 rF (.D(d2), .CLK(clk), .Q(qf));
  sky130_fd_sc_hd__edfxtp_1 rX (.D(dx), .DE(qe), .CLK(clk), .Q(qx));
  sky130_fd_sc_hd__buf_1 g_z (.A(qx), .X(z));

  sky130_fd_sc_hd__nand2_1 g2 (.A(qe), .B(qf), .Y(n1));
  sky130_fd_sc_hd__inv_1 g3 (.A(n1), .Y(n2));
  sky130_fd_sc_hd__inv_1 g4 (.A(n2), .Y(n3));
  sky130_fd_sc_hd__inv_1 g5 (.A(n3), .Y(n4));
  sky130_fd_sc_hd__inv_1 g6 (.A(n4), .Y(n5));
  sky130_fd_sc_hd__inv_1 g7 (.A(n5), .Y(n6));
  sky130_fd_sc_hd__and2_1 g8 (.A(n6), .B(rst_n), .X(d3));
  sky130_fd_sc_hd__dfxtp_1 r3 (.D(d3), .CLK(clk), .Q(q3));
  sky130_fd_sc_hd__buf_1 g9 (.A(q3), .X(y));

  sky130_fd_sc_hd__and2_1 g10 (.A(d), .B(rst_n), .X(d4));
  sky130_fd_sc_hd__and2_1 g11 (.A(e), .B(rst_n), .X(d5));
  sky130_fd_sc_hd__dfxtp_1 r4 (.D(d4), .CLK(clk), .Q(q4));
  sky130_fd_sc_hd__dfxtp_1 r5 (.D(d5), .CLK(clk), .Q(q5));
  sky130_fd_sc_hd__nor2_1 g12 (.A(q4), .B(q5), .Y(m1));
  sky130_fd_sc_hd__inv_1 g13 (.A(m1), .Y(m2));
  sky130_fd_sc_hd__inv_1 g14 (.A(m2), .Y(m3));
  sky130_fd_sc_hd__inv_1 g15 (.A(m3), .Y(m4));
  sky130_fd_sc_hd__inv_1 g16 (.A(m4), .Y(m5));
  sky130_fd_sc_hd__inv_1 g17 (.A(m5), .Y(m6));
  sky130_fd_sc_hd__and2_1 g18 (.A(m6), .B(rst_n), .X(d6));
  sky130_fd_sc_hd__dfxtp_1 r6 (.D(d6), .CLK(clk), .Q(q6));
  sky130_fd_sc_hd__buf_1 g19 (.A(q6), .X(w));
endmodule
