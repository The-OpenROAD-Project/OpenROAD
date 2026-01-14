module top (in,
    out);
 input in;
 output out;


 H0 h0 (.in(in),
    .out(out));
endmodule
module H0 (in,
    out);
 input in;
 output out;

 wire net1;

 BUF_X4 buf1 (.A(net1),
    .Z(out));
 BUF_X1 drvr (.A(in),
    .Z(net1));
endmodule
