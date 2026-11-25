// rc drives only the enable pin of r2, a control pin, so no edge of the
// retiming graph leaves i4.  Crossing i4 would take r1 from in front of it
// and put no register behind it: r2 would see its enable one cycle early.
// i4 is never crossed.
// rst_n is a synchronous reset in logic, so every register settles to a
// defined value for the sequential equivalence check.
module top (a, b, rst_n, clk, y);
  input a;
  input b;
  input rst_n;
  input clk;
  output y;
  wire d1, q1, n1, n2, n3, n4, qc, d2, q2;

  sky130_fd_sc_hd__and2_1 g0 (.A(a), .B(rst_n), .X(d1));
  sky130_fd_sc_hd__dfxtp_1 r1 (.D(d1), .CLK(clk), .Q(q1));
  sky130_fd_sc_hd__inv_1 i1 (.A(q1), .Y(n1));
  sky130_fd_sc_hd__inv_1 i2 (.A(n1), .Y(n2));
  sky130_fd_sc_hd__inv_1 i3 (.A(n2), .Y(n3));
  sky130_fd_sc_hd__inv_1 i4 (.A(n3), .Y(n4));
  sky130_fd_sc_hd__dfxtp_1 rc (.D(n4), .CLK(clk), .Q(qc));
  sky130_fd_sc_hd__and2_1 g1 (.A(b), .B(rst_n), .X(d2));
  sky130_fd_sc_hd__edfxtp_1 r2 (.D(d2), .DE(qc), .CLK(clk), .Q(q2));
  sky130_fd_sc_hd__buf_1 g9 (.A(q2), .X(y));
endmodule
