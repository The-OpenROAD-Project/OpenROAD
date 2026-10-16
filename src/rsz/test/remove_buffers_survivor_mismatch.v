// Both nets around b1 have top-level ports; two dangling outputs exercise
// selected and automatic removal without requiring additional routing.
module top (input in1, input in2, output out1, output out2);
  BUF_X1 b1 (.A(in1), .Z(out1));
  BUF_X1 u1 (.A(out1), .Z(out2));
  BUF_X1 dangling_auto (.A(in1), .Z());
  BUF_X1 dangling_selected (.A(in2), .Z());
endmodule
