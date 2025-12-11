module top (in,
    out);
 input in;
 output out;


 H0 h0 (.in(in),
    .out(out));
 BUF_X1 load1 (.A(out));
 BUF_X1 nontarget0 (.A(out));
endmodule
module H0 (in,
    out);
 input in;
 output out;


 BUF_X1 drvr (.A(in),
    .Z(out));
 BUF_X1 load0 (.A(in));
endmodule
