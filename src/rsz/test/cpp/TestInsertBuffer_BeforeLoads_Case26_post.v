module top (in);
 input in;

 wire Z_net_conn;
 wire n2;

 H0 h0 (.Z_net(Z_net_conn),
    .h0_in(in),
    .h0_out(n2));
 H1 h1 (.A_net(Z_net_conn),
    .h1_in(n2));
endmodule
module H0 (Z_net,
    h0_in,
    h0_out);
 output Z_net;
 input h0_in;
 output h0_out;


 BUF_X1 buf0 (.A(Z_net),
    .Z(h0_out));
 BUF_X1 drvr (.A(h0_in),
    .Z(Z_net));
 BUF_X1 nontarget0 (.A(Z_net));
endmodule
module H1 (A_net,
    h1_in);
 input A_net;
 input h1_in;

 wire n3;
 wire net1;

 BUF_X1 buf1 (.A(h1_in),
    .Z(n3));
 BUF_X1 load0 (.A(net1));
 BUF_X1 load1 (.A(net1));
 BUF_X1 new_buf1 (.A(A_net),
    .Z(net1));
endmodule
