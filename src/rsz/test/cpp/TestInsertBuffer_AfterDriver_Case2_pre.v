module top (X);
 input X;


 BUF_X1 load0_inst (.A(X));
 BUF_X1 load2_inst (.A(X));
 MOD0 mi0 (.A(X));
endmodule
module MOD0 (A);
 input A;


 MOD1 mi1 (.A(A));
endmodule
module MOD1 (A);
 input A;


 BUF_X1 load1_inst (.A(A));
endmodule
