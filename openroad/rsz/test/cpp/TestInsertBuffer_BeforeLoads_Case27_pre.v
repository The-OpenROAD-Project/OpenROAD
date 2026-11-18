module top (in);
 input in;


 H1 h1 (.A(in));
endmodule
module H1 (A);
 input A;


 BUF_X1 load1 (.A(A));
endmodule
