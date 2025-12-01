module top (out);
 output out;

 wire net2;
 wire net1;

 H0 h0 (.net1_i(net2),
    .h0_out(net1));
 BUF_X1 load0 (.A(net2),
    .Z(out));
 BUF_X1 new_buf1 (.A(net1),
    .Z(net2));
 BUF_X1 non_target0 (.A(net1));
endmodule
module H0 (net1_i,
    h0_out);
 input net1_i;
 output h0_out;


 H1 h1 (.h1_out(h0_out));
 BUF_X1 load1 (.A(net1_i));
endmodule
module H1 (h1_out);
 output h1_out;


 LOGIC0_X1 h0_h1_drvr (.Z(h1_out));
endmodule
