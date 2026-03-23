module top (out);
 output out;

 wire n1;

 BUF_X1 drvr (.Z(n1));
 BUF_X1 load1 (.A(out));
 BUF_X4 new_buf1 (.A(n1),
    .Z(out));
 MOD1 u1 (.A(out));
endmodule
module MOD1 (A);
 input A;


 BUF_X1 load2 (.A(A));
endmodule
