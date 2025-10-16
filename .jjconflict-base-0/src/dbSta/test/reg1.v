module top (in1, in2, clk1, clk2, clk3, out);
  input in1, in2, clk1, clk2, clk3;
  output out;
  wire r1q, r2q, u1z, u2z;

  snl_ffqx1 r1 (.D(in1), .CP(clk1), .Q(r1q));
  snl_ffqx1 r2 (.D(in2), .CP(clk2), .Q(r2q));
  snl_bufx1 u1 (.A(r2q), .Z(u1z));
  snl_and02x1 u2 (.A(r1q), .B(u1z), .Z(u2z));
  snl_ffqx1 r3 (.D(u2z), .CP(clk3), .Q(out));
endmodule // top
