module top (in,
    out);
 input in;
 output out;

 wire n2;
 wire n1;
 wire net2;

 MOD0 mod0 (.out(n2),
    .in(n1));
 BUF_X4 new_buf22 (.A(n2),
    .Z(net2));
 assign out = net2;
endmodule
module MOD0 (out,
    in);
 output out;
 input in;

 wire n2;
 wire n1;
 wire net1;

 BUF_X1 buf0 (.A(net1),
    .Z(n2));
 BUF_X4 new_buf11 (.A(n1),
    .Z(net1));
 assign out = n2;
endmodule
