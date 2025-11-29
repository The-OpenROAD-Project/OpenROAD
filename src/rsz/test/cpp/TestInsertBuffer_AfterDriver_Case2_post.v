module top (X);
 input X;

 wire net1;

 BUF_X4 buf1 (.A(X),
    .Z(net1));
 BUF_X1 load0_inst (.A(net1));
 BUF_X1 load2_inst (.A(net1));
 MOD0 mi0 (.A(net1));
endmodule
module MOD0 (A);
 input A;


 MOD1 mi1 (.A(A));
endmodule
module MOD1 (A);
 input A;


 BUF_X1 load1_inst (.A(A));
endmodule
