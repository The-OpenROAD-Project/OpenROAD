module top (in1, in2, clk1, clk2, clk3, out);
  input in1, in2, clk1, clk2, clk3;
  output out;
  wire r1q, r2q, u1z, u2z;

  MOCK_SINGLE r1 (.A1(in1), .A2(clk1), .ZN(r1q));
  MOCK_SINGLE r2 (.A1(in2), .A2(clk2), .ZN(r2q));
  MOCK_DOUBLE u2 (.A1(r1q), .A2(u1z), .ZN(u2z));
  MOCK_DOUBLE r3 (.A1(u2z), .A2(clk3), .ZN(out));
endmodule // top
