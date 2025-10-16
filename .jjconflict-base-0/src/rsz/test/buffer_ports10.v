module top (in1, clk1, out1, out2);
  input in1, clk1;
  output out1, out2;

  DFF_X1 r1 (.D(in1), .CK(clk1), .Q(out1));
  assign out2 = out1;
endmodule // top
