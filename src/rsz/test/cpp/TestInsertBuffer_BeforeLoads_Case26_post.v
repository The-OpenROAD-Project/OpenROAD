module top (in);
 input in;

 wire n2;
 wire net;

 H0 h0 (.h0_in(in),
    .h0_out(n2),
    .new_port(net));
 H1 h1 (.h1_in(n2),
    .new_port(net));
endmodule
module H0 (h0_in, 
    h0_out,
    new_port);
 input h0_in;
 output h0_out;
 output new_port;

 wire n1;

 BUF_X1 drvr (.A(h0_in),
    .Z(n1));
 BUF_X1 nontarget0 (.A(n1));
 BUF_X1 buf0 (.A(n1),
    .Z(h0_out));
 assign new_port = n1;
endmodule
module H1 (h1_in,
    new_port);
 input h1_in;
 input new_port;

 wire n3;
 wire net2;

 BUF_X4 new_buf (.A(new_port),
    .Z(net2));
 BUF_X1 buf1 (.A(h1_in),
    .Z(n3));
 BUF_X1 load0 (.A(net2));
 BUF_X1 load1 (.A(net2));
endmodule
