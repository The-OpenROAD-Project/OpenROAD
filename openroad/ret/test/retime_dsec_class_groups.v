// Registers of different classes on one data net `wd`, as in a register
// file where every word reads the same write-data bit and has its own
// enable (sky130hd edfxtp):
//   r1  enable en1, long cone: it slides across b1 and the inverters and
//       stays an edfxtp enabled by en1;
//   r3  enable en1 as well: the same class as r1, rebuilt with it;
//   r2  enable en2, short cone;
//   r4  plain register.
// r1 moving does not unmap r2 or r4: each class on the net is rebuilt on its
// own, and no register here is unmapped.  rst_n is a synchronous reset in
// logic and the enables are forced on while it is asserted, so every
// register settles to a defined value for the sequential equivalence check.
module top (d, en1_i, en2_i, rst_n, clk, y, z, w, v);
  input d;
  input en1_i;
  input en2_i;
  input rst_n;
  input clk;
  output y;
  output z;
  output w;
  output v;
  wire rst, en1, en2, wd, q1, q2, q3, q4;
  wire n0, n1, n2, n3, n4, n5, d5, q5, m0, d6, q6, k0, d7, q7, p0, p1, d8, q8;

  sky130_fd_sc_hd__inv_1 g_r (.A(rst_n), .Y(rst));
  sky130_fd_sc_hd__or2_1 g_e1 (.A(en1_i), .B(rst), .X(en1));
  sky130_fd_sc_hd__or2_1 g_e2 (.A(en2_i), .B(rst), .X(en2));
  sky130_fd_sc_hd__and2_1 g0 (.A(d), .B(rst_n), .X(wd));
  sky130_fd_sc_hd__edfxtp_1 r1 (.D(wd), .DE(en1), .CLK(clk), .Q(q1));
  sky130_fd_sc_hd__edfxtp_1 r2 (.D(wd), .DE(en2), .CLK(clk), .Q(q2));
  sky130_fd_sc_hd__edfxtp_1 r3 (.D(wd), .DE(en1), .CLK(clk), .Q(q3));
  sky130_fd_sc_hd__dfxtp_1 r4 (.D(wd), .CLK(clk), .Q(q4));

  // r1: the long cone.
  sky130_fd_sc_hd__buf_1 b1 (.A(q1), .X(n0));
  sky130_fd_sc_hd__inv_1 g3 (.A(n0), .Y(n1));
  sky130_fd_sc_hd__inv_1 g4 (.A(n1), .Y(n2));
  sky130_fd_sc_hd__inv_1 g5 (.A(n2), .Y(n3));
  sky130_fd_sc_hd__inv_1 g6 (.A(n3), .Y(n4));
  sky130_fd_sc_hd__inv_1 g7 (.A(n4), .Y(n5));
  sky130_fd_sc_hd__and2_1 g8 (.A(n5), .B(rst_n), .X(d5));
  sky130_fd_sc_hd__dfxtp_1 r5 (.D(d5), .CLK(clk), .Q(q5));
  sky130_fd_sc_hd__buf_1 g9 (.A(q5), .X(y));

  // r2: another enable, short cone.
  sky130_fd_sc_hd__buf_1 b2 (.A(q2), .X(m0));
  sky130_fd_sc_hd__and2_1 g18 (.A(m0), .B(rst_n), .X(d6));
  sky130_fd_sc_hd__dfxtp_1 r6 (.D(d6), .CLK(clk), .Q(q6));
  sky130_fd_sc_hd__buf_1 g19 (.A(q6), .X(z));

  // r3: the class of r1, short cone.
  sky130_fd_sc_hd__inv_1 g21 (.A(q3), .Y(k0));
  sky130_fd_sc_hd__and2_1 g28 (.A(k0), .B(rst_n), .X(d7));
  sky130_fd_sc_hd__dfxtp_1 r7 (.D(d7), .CLK(clk), .Q(q7));
  sky130_fd_sc_hd__buf_1 g29 (.A(q7), .X(w));

  // r4: plain, short cone.
  sky130_fd_sc_hd__inv_1 g31 (.A(q4), .Y(p0));
  sky130_fd_sc_hd__inv_1 g32 (.A(p0), .Y(p1));
  sky130_fd_sc_hd__and2_1 g38 (.A(p1), .B(rst_n), .X(d8));
  sky130_fd_sc_hd__dfxtp_1 r8 (.D(d8), .CLK(clk), .Q(q8));
  sky130_fd_sc_hd__buf_1 g39 (.A(q8), .X(v));
endmodule
