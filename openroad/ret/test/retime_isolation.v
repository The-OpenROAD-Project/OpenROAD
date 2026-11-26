// r1 and rs feed an isolation cell; the path behind it fails.  Sliding them
// across the cell would put the register on the other side of a power
// domain boundary.
module top (a, s, rst_n, clk, y);
  input a;
  input s;
  input rst_n;
  input clk;
  output y;
  wire d1, q1, ds, qs, x, n1, n2, n3, n4, n5, n6, q2;

  sky130_fd_sc_hd__and2_1 g0 (.A(a), .B(rst_n), .X(d1));
  sky130_fd_sc_hd__dfxtp_1 r1 (.D(d1), .CLK(clk), .Q(q1));
  sky130_fd_sc_hd__and2_1 g1 (.A(s), .B(rst_n), .X(ds));
  sky130_fd_sc_hd__dfxtp_1 rs (.D(ds), .CLK(clk), .Q(qs));
  sky130_fd_sc_hd__lpflow_inputiso0p_1 iso (.A(q1), .SLEEP(qs), .X(x));
  sky130_fd_sc_hd__inv_1 i1 (.A(x), .Y(n1));
  sky130_fd_sc_hd__inv_1 i2 (.A(n1), .Y(n2));
  sky130_fd_sc_hd__inv_1 i3 (.A(n2), .Y(n3));
  sky130_fd_sc_hd__inv_1 i4 (.A(n3), .Y(n4));
  sky130_fd_sc_hd__inv_1 i5 (.A(n4), .Y(n5));
  sky130_fd_sc_hd__inv_1 i6 (.A(n5), .Y(n6));
  sky130_fd_sc_hd__dfxtp_1 r2 (.D(n6), .CLK(clk), .Q(q2));
  sky130_fd_sc_hd__buf_1 g9 (.A(q2), .X(y));
endmodule
