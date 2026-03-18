module top (in,
    out);
 input in;
 output out;

 wire net1;

 H0 h0 (.in_2(net1),
    .in(in),
    .out(out));
 BUF_X1 load1 (.A(net1));
 BUF_X1 new_buf1 (.A(out),
    .Z(net1));
 BUF_X1 nontarget0 (.A(out));
endmodule
module H0 (in_2,
    in,
    out);
 input in_2;
 input in;
 output out;


 BUF_X1 drvr (.A(in),
    .Z(out));
 BUF_X1 load0 (.A(in_2));
endmodule
