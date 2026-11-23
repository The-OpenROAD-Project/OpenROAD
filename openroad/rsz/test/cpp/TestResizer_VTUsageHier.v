module inner (in,
    out);
 input in;
 output out;

 wire n0;

 BUF_X16 b2 (.A(in),
    .Z(n0));
 INV_X1 i1 (.A(n0),
    .ZN(out));
endmodule

module sub (in,
    out);
 input in;
 output out;

 wire n0;
 wire n1;

 BUF_X1 b1 (.A(in),
    .Z(n0));
 NAND2_X1 n2 (.A1(n0),
    .A2(in),
    .ZN(n1));
 inner u_inner (.in(n1),
    .out(out));
endmodule

module top (in,
    out);
 input in;
 output out;

 wire n0;
 wire n1;

 BUF_X1 b0 (.A(in),
    .Z(n0));
 INV_X1 i0 (.A(n0),
    .ZN(n1));
 sub u_sub (.in(n1),
    .out(out));
endmodule
