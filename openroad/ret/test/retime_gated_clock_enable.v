// i4 drives the GATE pin of the clock gate icg.  Crossing i4 would take re
// from in front of it and put no register behind it: the clock of r1 would
// be gated one cycle early.  i4 is never crossed.
module top (a, b, rst_n, clk, y);
  input a;
  input b;
  input rst_n;
  input clk;
  output y;
  wire de, qe, e1, e2, e3, e4, gclk, d1, q1;

  sky130_fd_sc_hd__and2_1 g0 (.A(a), .B(rst_n), .X(de));
  sky130_fd_sc_hd__dfxtp_1 re (.D(de), .CLK(clk), .Q(qe));
  sky130_fd_sc_hd__inv_1 i1 (.A(qe), .Y(e1));
  sky130_fd_sc_hd__inv_1 i2 (.A(e1), .Y(e2));
  sky130_fd_sc_hd__inv_1 i3 (.A(e2), .Y(e3));
  sky130_fd_sc_hd__inv_1 i4 (.A(e3), .Y(e4));
  sky130_fd_sc_hd__dlclkp_1 icg (.CLK(clk), .GATE(e4), .GCLK(gclk));
  sky130_fd_sc_hd__and2_1 g1 (.A(b), .B(rst_n), .X(d1));
  sky130_fd_sc_hd__dfxtp_1 r1 (.D(d1), .CLK(gclk), .Q(q1));
  sky130_fd_sc_hd__buf_1 g9 (.A(q1), .X(y));
endmodule
