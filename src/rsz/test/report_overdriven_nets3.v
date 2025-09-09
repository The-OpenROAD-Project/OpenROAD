module top (in1, in2, en1, en2, out);
  input in1, in2, en1, en2;
  output out;

  sky130_fd_sc_hs__ebufn_4 u0 (.A(in1), .TE_B(en1), .Z(out));
  sky130_fd_sc_hs__ebufn_4 u1 (.A(in2), .TE_B(en2), .Z(out));

endmodule // top
