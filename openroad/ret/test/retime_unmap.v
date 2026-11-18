// The register that should move is an enable flop; retime unmaps it to a
// plain flop plus a mux cone first, then slides the plain flop forward.
module top (a, b, en, clk, y);
  input a;
  input b;
  input en;
  input clk;
  output y;
  wire n0, q1, n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12, n13, n14, q2;

  sky130_fd_sc_hd__and2_1 g0 (.A(a), .B(b), .X(n0));
  sky130_fd_sc_hd__edfxtp_1 r1 (.D(n0), .DE(en), .CLK(clk), .Q(q1));
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
endmodule
