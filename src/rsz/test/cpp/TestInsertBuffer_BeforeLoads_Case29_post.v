module top (in);
 input in;

 wire net1;
 wire n1;

 BUF_X1 drvr (.A(in),
    .Z(n1));
 H0 h0 (.h0_in(net1));
 BUF_X1 load0 (.A(net1));
 BUF_X4 new_buf1 (.A(n1),
    .Z(net1));
endmodule
module H0 (h0_in);
 input h0_in;


 BUF_X1 load1 (.A(h0_in));
endmodule
