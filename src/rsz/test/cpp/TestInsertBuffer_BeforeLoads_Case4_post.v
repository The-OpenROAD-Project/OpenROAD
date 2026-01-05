module top ();

 wire n1;
 wire net1;
 wire net2;

 BUF_X1 drvr0 (.Z(n1));
 BUF_X1 load0 (.A(net1));
 BUF_X1 load1 (.A(net1));
 BUF_X1 load2 (.A(net2));
 BUF_X4 new01 (.A(net2),
    .Z(net1));
 BUF_X4 new12 (.A(n1),
    .Z(net2));
endmodule
