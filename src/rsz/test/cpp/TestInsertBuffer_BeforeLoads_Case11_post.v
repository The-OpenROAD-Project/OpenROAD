module top (in);
 input in;

 wire n1;
 wire net1;

 BUF_X1 load1 (.A(net1));
 BUF_X1 load2 (.A(n1));
 BUF_X4 new_buf1 (.A(n1),
    .Z(net1));
endmodule
