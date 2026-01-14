module top (in);
 input in;

 wire net4;
 wire n2;

 H0 h0 (.net2(net4),
    .h0_in(in),
    .h0_out(n2));
 H1 h1 (.net3(net4),
    .h1_in(n2));
endmodule
module H0 (net2,
    h0_in,
    h0_out);
 output net2;
 input h0_in;
 output h0_out;


 BUF_X1 buf0 (.A(net2),
    .Z(h0_out));
 BUF_X1 drvr (.A(h0_in),
    .Z(net2));
 BUF_X1 nontarget0 (.A(net2));
endmodule
module H1 (net3,
    h1_in);
 input net3;
 input h1_in;

 wire n3;
 wire net1;

 BUF_X1 buf1 (.A(h1_in),
    .Z(n3));
 BUF_X1 load0 (.A(net1));
 BUF_X1 load1 (.A(net1));
 BUF_X1 new_buf1 (.A(net3),
    .Z(net1));
endmodule
