module top (in,
    out);
 input in;
 output out;

 wire n1;
 wire n2;
 wire net1;
 wire net2;

 BUF_X1 buf0 (.A(net1),
    .Z(n2));
 BUF_X4 buf1 (.A(n1),
    .Z(net1));
 BUF_X4 buf2 (.A(n2),
    .Z(net2));
 assign out = net2;
endmodule
