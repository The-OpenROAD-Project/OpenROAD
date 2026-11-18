// b1 is a hierarchical feedthrough; its input flat net has no top port,
// while the output flat net does. Both must select the same survivor.
module top (input in1, input in2, output out1, output out2);
  wire internal;
  AND2_X1 driver (.A1(in1), .A2(in2), .ZN(internal));
  feedthrough u (.i(internal), .o(out1));
  BUF_X1 u1 (.A(out1), .Z(out2));
endmodule

module feedthrough (input i, output o);
  BUF_X1 b1 (.A(i), .Z(o));
endmodule
