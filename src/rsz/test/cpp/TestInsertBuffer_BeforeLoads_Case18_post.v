module top (in);
 input in;

 wire net1;
 wire n2;
 wire n1;

 BUF_X1 buf (.A(n1),
    .Z(n2));
 BUF_X1 drvr (.A(in),
    .Z(n1));
 H0 h0 (.n2(net1),
    .h0_in(n2));
 BUF_X1 load0 (.A(net1));
 BUF_X4 new_buf1 (.A(n1),
    .Z(net1));
 BUF_X1 non_target0 (.A(n1));
endmodule
module H0 (n2,
    h0_in);
 input n2;
 input h0_in;


 BUF_X1 load1 (.A(n2));
 BUF_X1 non_target1 (.A(h0_in));
endmodule
