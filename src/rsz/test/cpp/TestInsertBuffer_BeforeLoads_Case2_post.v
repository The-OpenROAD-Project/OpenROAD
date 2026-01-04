module top (in,
    out);
 input in;
 output out;

 wire net2;
 wire net1;

 BUF_X1 buf0 (.A(net1),
    .Z(net2));
 BUF_X4 buf1 (.A(in),
    .Z(net1));
 BUF_X4 buf2 (.A(net2),
    .Z(out));
endmodule
