module top (in);
 input in;


 H1 h1 (.A(in));
endmodule
module H1 (A);
 input A;

 wire net1;

 BUF_X1 load1 (.A(net1));
 BUF_X4 new_buf1 (.A(A),
    .Z(net1));
endmodule
