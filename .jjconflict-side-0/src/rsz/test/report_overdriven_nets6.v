module top (in1, in2, en1, en2, out);
  input in1, in2, en1, en2;
  output out;

  sky130_fd_sc_hs__inv_1 u0 (.A(in1), .Y(out));
  sky130_fd_sc_hs__inv_1 u1 (.A(in2), .Y(out));

endmodule // top
