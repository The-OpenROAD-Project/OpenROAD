module top (in);
 input in;

 wire net4;
 wire n2;

 H0 h0 (.\path/data_5__2 (net4),
    .h0_in(in),
    .h0_out(n2));
 H1 h1 (.net3(net4),
    .h1_in(n2));
endmodule
module H0 (\path/data_5__2 ,
    h0_in,
    h0_out);
 output \path/data_5__2 ;
 input h0_in;
 output h0_out;

 wire collision_output;
 wire [5:0] \path/data ;

 BUF_X1 buf0 (.A(\path/data [5]),
    .Z(h0_out));
 BUF_X1 drvr (.A(h0_in),
    .Z(\path/data [5]));
 BUF_X1 nontarget0 (.A(\path/data [5]));
 BUF_X1 \path/data_5_  (.A(\path/data [5]),
    .Z(collision_output));
 assign \path/data_5__2  = \path/data [5];
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
