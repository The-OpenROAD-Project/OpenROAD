module top (in1,
    out1);
 input in1;
 output out1;


 BUF_X1 u1 (.A(in1),
    .Z(out1));
endmodule
