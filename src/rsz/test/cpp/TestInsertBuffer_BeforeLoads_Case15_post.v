module top (in,
    out);
 input in;
 output out;

 wire net1;
 wire n1;

 BUF_X1 drvr (.A(in),
    .Z(n1));
 BUF_X1 load1 (.A(net1));
 BUF_X1 new_buf1 (.A(n1),
    .Z(net1));
 MOD1 u1 (.n1(net1),
    .A(n1));
 assign out = n1;
endmodule
module MOD1 (n1,
    A);
 input n1;
 input A;


 BUF_X1 load2 (.A(n1));
 BUF_X1 load3 (.A(n1));
 BUF_X1 non_target (.A(A));
endmodule
