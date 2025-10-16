module top (in1, clk1, out1);
  input in1, clk1;
  output out1;

  DFF_X1 r1 (.D(in1), .CK(clk1), .Q(out));
endmodule // top
