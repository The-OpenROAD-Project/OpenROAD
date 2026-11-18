module top ();

 wire n1;
 wire n2;
 wire net1;

 BUF_X1 buf0 (.A(net1),
    .Z(n2));
 BUF_X4 buf1 (.A(n1),
    .Z(net1));
 BUF_X1 drvr (.Z(n1));
 BUF_X1 load (.A(n2));
endmodule
