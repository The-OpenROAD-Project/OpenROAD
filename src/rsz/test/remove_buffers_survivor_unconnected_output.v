// Both buffers have inputs but no connected outputs. They are safe to remove.
module top (input in1, input in2, output out1);
  BUF_X1 b1 (.A(in1), .Z());
  BUF_X1 b2 (.A(in2), .Z());
  AND2_X1 keep (.A1(in1), .A2(in2), .ZN(out1));
endmodule
