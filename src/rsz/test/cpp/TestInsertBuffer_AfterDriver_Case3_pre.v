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


 BUF_X1 drvr (.A(in),
    .Z(out));
endmodule
