module top (in);
 input in;

 wire net1;
 wire w1;
 wire n1;

 BUF_X1 drvr (.A(in),
    .Z(n1));
 H0 h0 (.in(net1),
    .out(w1));
 H1 h1 (.in(net1));
 BUF_X1 new_buf1 (.A(n1),
    .Z(net1));
endmodule
module H0 (in,
    out);
 input in;
 output out;


 BUF_X1 internal_buf (.A(in));
 assign out = in;
endmodule
module H1 (in);
 input in;


 BUF_X1 internal_load (.A(in));
endmodule
