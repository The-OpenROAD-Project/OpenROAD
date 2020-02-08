module top (in1, in2, clk, out);
  input in1, in2, clk;
  output out;
  wire clk1, clk2, clk3, r1q, r2q, u1z, u2z;

  snl_bufx1 i1 (.A(clk), .Z(i1z));
  snl_bufx1 i2 (.A(i1z), .Z(i2z));
  snl_bufx1 i3 (.A(i2z), .Z(i3z));
  snl_bufx1 i4 (.A(i3z), .Z(i4z));
  snl_bufx1 i5 (.A(i4z), .Z(i5z));
  snl_bufx1 i6 (.A(i5z), .Z(i6z));
  snl_ffqx4 r1 (.D(in1), .CP(i1z), .Q(r1q));
  snl_ffqx4 r2 (.D(in2), .CP(i4z), .Q(r2q));
  snl_bufx1 u1 (.A(r2q), .Z(u1z));
  snl_and02x1 u2 (.A(r1q), .B(u1z), .Z(u2z));
  snl_ffqx4 r3 (.D(u2z), .CP(i5z), .Q(out));
endmodule // top
