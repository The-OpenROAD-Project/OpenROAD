// r3 (sky130hd edfxtp) has its enable pin DE unconnected, so its next state
// cannot be rebuilt from library cells.  r3 and the plain r4 feed g20: a
// mixed-class gate, so a slide across it would unmap r3.  r1 starts an
// independent plain cone that fails by more.
module top (a, b, c, d, clk, y, z);
  input a;
  input b;
  input c;
  input d;
  input clk;
  output y;
  output z;
  wire d1, q1, n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12, n13, n14;
  wire q2, d3, d4, q3, q4, m0, m1, m2, m3, m4, m5, m6, q5;

  // Plain cone: r1 -> 14 inverters -> r2.
  sky130_fd_sc_hd__and2_1 g0 (.A(a), .B(b), .X(d1));
  sky130_fd_sc_hd__dfxtp_1 r1 (.D(d1), .CLK(clk), .Q(q1));
  sky130_fd_sc_hd__inv_1 g1 (.A(q1), .Y(n1));
  sky130_fd_sc_hd__inv_1 g2 (.A(n1), .Y(n2));
  sky130_fd_sc_hd__inv_1 g3 (.A(n2), .Y(n3));
  sky130_fd_sc_hd__inv_1 g4 (.A(n3), .Y(n4));
  sky130_fd_sc_hd__inv_1 g5 (.A(n4), .Y(n5));
  sky130_fd_sc_hd__inv_1 g6 (.A(n5), .Y(n6));
  sky130_fd_sc_hd__inv_1 g7 (.A(n6), .Y(n7));
  sky130_fd_sc_hd__inv_1 g8 (.A(n7), .Y(n8));
  sky130_fd_sc_hd__inv_1 g9 (.A(n8), .Y(n9));
  sky130_fd_sc_hd__inv_1 g10 (.A(n9), .Y(n10));
  sky130_fd_sc_hd__inv_1 g11 (.A(n10), .Y(n11));
  sky130_fd_sc_hd__inv_1 g12 (.A(n11), .Y(n12));
  sky130_fd_sc_hd__inv_1 g13 (.A(n12), .Y(n13));
  sky130_fd_sc_hd__inv_1 g14 (.A(n13), .Y(n14));
  sky130_fd_sc_hd__dfxtp_1 r2 (.D(n14), .CLK(clk), .Q(q2));
  sky130_fd_sc_hd__buf_1 g15 (.A(q2), .X(y));

  // Mixed-class cone: r3 (DE unconnected) and r4 -> NAND -> 6 inverters
  // -> r5.
  sky130_fd_sc_hd__and2_1 g18 (.A(c), .B(b), .X(d3));
  sky130_fd_sc_hd__and2_1 g19 (.A(d), .B(b), .X(d4));
  sky130_fd_sc_hd__edfxtp_1 r3 (.D(d3), .CLK(clk), .Q(q3));
  sky130_fd_sc_hd__dfxtp_1 r4 (.D(d4), .CLK(clk), .Q(q4));
  sky130_fd_sc_hd__nand2_1 g20 (.A(q3), .B(q4), .Y(m0));
  sky130_fd_sc_hd__inv_1 g21 (.A(m0), .Y(m1));
  sky130_fd_sc_hd__inv_1 g22 (.A(m1), .Y(m2));
  sky130_fd_sc_hd__inv_1 g23 (.A(m2), .Y(m3));
  sky130_fd_sc_hd__inv_1 g24 (.A(m3), .Y(m4));
  sky130_fd_sc_hd__inv_1 g25 (.A(m4), .Y(m5));
  sky130_fd_sc_hd__inv_1 g26 (.A(m5), .Y(m6));
  sky130_fd_sc_hd__dfxtp_1 r5 (.D(m6), .CLK(clk), .Q(q5));
  sky130_fd_sc_hd__buf_1 g27 (.A(q5), .X(z));
endmodule
