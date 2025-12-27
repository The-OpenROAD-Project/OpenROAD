module top (out);
 output out;

 wire net1;
 wire n1;

 BUF_X1 drvr (.Z(n1));
 BUF_X1 load1 (.A(net1));
 BUF_X4 new_buf1 (.A(n1),
    .Z(net1));
 MOD1 u1 (.A(net1));
 assign out = net1;
endmodule
module MOD1 (A);
 input A;


 BUF_X1 load2 (.A(A));
endmodule
