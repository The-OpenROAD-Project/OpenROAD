module top (in,
    out1,
    out2,
    out_net);
 input in;
 output out1;
 output out2;
 output out_net;

 wire n_internal;

 BUF_X1 extra (.A(n_internal),
    .Z(out_net));
 SUB sub0 (.A(in),
    .net(n_internal),
    .Z1(out1),
    .Z2(out2));
endmodule
module SUB (A,
    net,
    Z1,
    Z2);
 input A;
 output net;
 output Z1;
 output Z2;

 wire net1;

 BUF_X1 drv (.A(A),
    .Z(net));
 BUF_X1 load1 (.A(net),
    .Z(Z1));
 BUF_X1 load2 (.A(net1),
    .Z(Z2));
 BUF_X4 split (.A(net),
    .Z(net1));
endmodule
