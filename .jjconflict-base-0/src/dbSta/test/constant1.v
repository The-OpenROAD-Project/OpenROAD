module top (in1, clk1, out1);
  input in1, clk1;
  output out1;

  AND2_X1 u1 (.A1(in1), .A2(1'b0), .ZN(u1z));
  DFF_X1 r1 (.D(u1z), .CK(clk1), .Q(r1q));
  DFF_X1 r2 (.D(r1q), .CK(clk1), .Q(out1));
endmodule // top
