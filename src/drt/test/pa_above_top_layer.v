module top (in, out);
  input in;
  output out;
  sky130_fd_sc_hd__probec_p_8 u1 (.A(in), .X(out));
endmodule
