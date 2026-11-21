// A register chain that only loses its last register.  rA (plain) feeds rB
// (enable `en`); rB and rC (enable `en`) feed one NAND and merge into a single
// edfxtp after it.  rA stays in front of the NAND and must be rebuilt as a
// plain register, not as an enable register.  rD (plain) shares rA's data net,
// so the two are rebuilt as one register.  rst_n is a synchronous reset in
// logic and the enable is forced on while it is asserted, so every register
// settles to a defined value for the sequential equivalence check.
module top (a, b, en_i, rst_n, clk, y, z);
  input a;
  input b;
  input en_i;
  input rst_n;
  input clk;
  output y;
  output z;
  wire rst, en, d1, d2, qa, qb, qc, qd;
  wire n1, n2, n3, n4, n5, n6, d3, q3;

  sky130_fd_sc_hd__inv_1 g_r (.A(rst_n), .Y(rst));
  sky130_fd_sc_hd__or2_1 g_e1 (.A(en_i), .B(rst), .X(en));
  sky130_fd_sc_hd__and2_1 g0 (.A(a), .B(rst_n), .X(d1));
  sky130_fd_sc_hd__and2_1 g1 (.A(b), .B(rst_n), .X(d2));
  sky130_fd_sc_hd__dfxtp_1 rA (.D(d1), .CLK(clk), .Q(qa));
  sky130_fd_sc_hd__edfxtp_1 rB (.D(qa), .DE(en), .CLK(clk), .Q(qb));
  sky130_fd_sc_hd__edfxtp_1 rC (.D(d2), .DE(en), .CLK(clk), .Q(qc));
  sky130_fd_sc_hd__dfxtp_1 rD (.D(d1), .CLK(clk), .Q(qd));
  sky130_fd_sc_hd__buf_1 g_z (.A(qd), .X(z));

  // rB, rC -> NAND -> inverters -> r3.
  sky130_fd_sc_hd__nand2_1 g2 (.A(qb), .B(qc), .Y(n1));
  sky130_fd_sc_hd__inv_1 g3 (.A(n1), .Y(n2));
  sky130_fd_sc_hd__inv_1 g4 (.A(n2), .Y(n3));
  sky130_fd_sc_hd__inv_1 g5 (.A(n3), .Y(n4));
  sky130_fd_sc_hd__inv_1 g6 (.A(n4), .Y(n5));
  sky130_fd_sc_hd__inv_1 g7 (.A(n5), .Y(n6));
  sky130_fd_sc_hd__and2_1 g8 (.A(n6), .B(rst_n), .X(d3));
  sky130_fd_sc_hd__dfxtp_1 r3 (.D(d3), .CLK(clk), .Q(q3));
  sky130_fd_sc_hd__buf_1 g9 (.A(q3), .X(y));
endmodule
