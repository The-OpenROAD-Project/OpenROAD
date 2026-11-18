module testcase(clk, din, dout);
  input clk;
  input din;
  output dout;

  DFF_X1 _0_ (
    .CK(clk),
    .D(din),
    .Q(dout)
  );
endmodule
