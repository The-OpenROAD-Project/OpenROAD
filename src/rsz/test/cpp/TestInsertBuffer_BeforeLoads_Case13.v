module top (out);
 output out;

 wire n1;

 BUF_X1 drvr (.Z(n1));
 BUF_X1 load1 (.A(n1));
 MOD1 u1 (.A(n1));
 assign out = n1;
endmodule
module MOD1 (A);
 input A;


 BUF_X1 load2 (.A(A));
endmodule
