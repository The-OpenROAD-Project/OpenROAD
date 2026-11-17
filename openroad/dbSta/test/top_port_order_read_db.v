module top (a, en, z);
  input [1:0] a;
  input en;
  output z;
  AND3_X1 u1 (.A1(a[0]), .A2(a[1]), .A3(en), .ZN(z));
endmodule
