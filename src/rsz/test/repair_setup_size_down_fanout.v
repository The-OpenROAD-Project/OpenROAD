module reg1 (clk);
 input clk;

 wire r1q;
 wire r1qb;
 wire r1qc;
 wire u1z;
 wire u2z;
 wire u3z;
 wire u4z;
 wire u5z;

 DFF_X1 r1 (.CK(clk),
    .Q(r1q));
 BUF_X8 u1 (.A(r1q),
    .Z(u1z));
 BUF_X1 u2 (.A(u1z),
    .Z(u2z));
 BUF_X1 u3 (.A(u2z),
    .Z(u3z));
 BUF_X1 u4 (.A(u3z),
    .Z(u4z));
 BUF_X1 u5 (.A(u4z),
    .Z(u5z));
 DFF_X1 r2 (.D(u5z),
    .CK(clk));
 DFF_X1 r3 (.D(r1q),
    .CK(clk));
 BUF_X8 r4a (.A(r1q),
    .Z(r1qb));
 DFF_X2 r4 (.D(r1qb),
    .CK(clk));
 BUF_X8 r5a (.A(r1q),
    .Z(r1qc));
 DFF_X2 r5 (.D(r1qc),
    .CK(clk));
 DFF_X2 r6 (.D(r1q),
    .CK(clk));
 DFF_X2 r7 (.D(r1q),
    .CK(clk));
endmodule
