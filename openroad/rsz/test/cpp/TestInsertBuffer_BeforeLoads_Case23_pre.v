module top (in,
    out);
 input in;
 output out;

 wire n1;

 DFF_X1 drvr (.D(in),
    .CK(in),
    .Q(n1));
 m1 inst1 (.p1(n1),
    .out(out));
endmodule
module m1 (p1,
    out);
 input p1;
 output out;

 wire n1; // Unrelated net with same name as top net
 wire n2;

 BUF_X1 load_inst (.A(p1),
    .Z(n2));
 BUF_X1 other_inst (.A(n2),
    .Z(n1));
 BUF_X1 out_inst (.A(n1),
    .Z(out));
endmodule
