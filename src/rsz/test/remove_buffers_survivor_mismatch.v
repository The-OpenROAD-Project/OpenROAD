// Two top-level port nets around b1, and a second buffer on out1.
module top (input in1, output out1, output out2);
  BUF_X1 b1 (.A(in1), .Z(out1));
  BUF_X1 u1 (.A(out1), .Z(out2));
endmodule
