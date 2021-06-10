module top (in1, in2, clk1, clk2, clk3, out, reset);
  input in1, in2, clk1, clk2, clk3, reset;
  output out;
  wire r1q, r2q, u1z, u2z;

  sky130_fd_sc_hd__dfrtn_1 r1 (.D(in1), .CLK_N(clk1), .Q(r1q), .RESET_B(reset));
  sky130_fd_sc_hd__dfrtn_1 r2 (.D(in2), .CLK_N(clk2), .Q(r2q), .RESET_B(reset));
  sky130_fd_sc_hd__buf_1 u1 (.A(r2q), .X(u1z));
  sky130_fd_sc_hd__and2_0 u2 (.A(r1q), .B(u1z), .X(u2z));
  sky130_fd_sc_hd__dfrtn_1 r3 (.D(u2z), .CLK_N(clk3), .Q(out), .RESET_B(reset));
endmodule // top
