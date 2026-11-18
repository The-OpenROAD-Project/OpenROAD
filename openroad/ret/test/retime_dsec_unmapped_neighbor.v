// Registers retimed forward across gates the unmapper built for another
// register.  rD drives the data and rA the enable of rB (sky130hd edfxtp).
// rB and the plain register rC feed the NOR g2: a mixed-class merge, so rB is
// unmapped into a base register whose D is (D & DE) | (state & !DE).  In the
// search that follows, rA no longer drives an enable pin: its register slides
// across the inverter the unmapper put on the enable path, then merges with
// rD's register across the D & DE gate, while rB's state stays the single
// register on its own feedback loop.  rst_n is a synchronous reset in logic
// and rA forces rB's enable on while it is asserted, so every register
// settles to a defined value for the sequential equivalence check.
module top (c, d, en_i, rst_n, clk, y);
  input c;
  input d;
  input en_i;
  input rst_n;
  input clk;
  output y;
  wire rst, da, dc, dd, qa, qb, qc, qd, n1, n2, dr, qr;

  sky130_fd_sc_hd__inv_1 g_r (.A(rst_n), .Y(rst));
  sky130_fd_sc_hd__or2_1 g_a (.A(en_i), .B(rst), .X(da));
  sky130_fd_sc_hd__and2_1 g_c (.A(c), .B(rst_n), .X(dc));
  sky130_fd_sc_hd__and2_1 g_d (.A(d), .B(rst_n), .X(dd));
  sky130_fd_sc_hd__dfxtp_1 rA (.D(da), .CLK(clk), .Q(qa));
  sky130_fd_sc_hd__dfxtp_1 rD (.D(dd), .CLK(clk), .Q(qd));
  sky130_fd_sc_hd__edfxtp_1 rB (.D(qd), .DE(qa), .CLK(clk), .Q(qb));
  sky130_fd_sc_hd__dfxtp_1 rC (.D(dc), .CLK(clk), .Q(qc));

  sky130_fd_sc_hd__nor2_1 g2 (.A(qb), .B(qc), .Y(n1));
  sky130_fd_sc_hd__inv_1 g3 (.A(n1), .Y(n2));
  sky130_fd_sc_hd__and2_1 g4 (.A(n2), .B(rst_n), .X(dr));
  sky130_fd_sc_hd__dfxtp_1 rR (.D(dr), .CLK(clk), .Q(qr));
  sky130_fd_sc_hd__buf_1 g5 (.A(qr), .X(y));
endmodule
