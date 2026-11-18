module top (in);
 input in;

 wire net;
 wire net1;

 BUF_X4 buf1 (.A(net1),
    .Z(net));
 BUF_X1 drvr_inst (.A(in),
    .Z(net1));
 BUF_X1 load0_inst (.A(net));
 BUF_X1 load2_inst (.A(net));
 MOD0 mi0 (.A(net));
endmodule
module MOD0 (A);
 input A;


 MOD1 mi1 (.A(A));
endmodule
module MOD1 (A);
 input A;


 BUF_X1 load1_inst (.A(A));
endmodule
